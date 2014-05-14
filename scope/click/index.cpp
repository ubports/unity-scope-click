/*
 * Copyright (C) 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#include <QDebug>
#include <QObject>
#include <QProcess>

#include <cstdlib>
#include <sstream>

#include "download-manager.h"
#include "index.h"
#include "interface.h"
#include "application.h"
#include "smartconnect.h"

namespace json = Json;

namespace click
{

void PackageManager::uninstall(const Package& package,
                               std::function<void(int, std::string)> callback)
{
    std::string package_id = package.name + ";" + package.version + ";all;local:click";
    std::string command = "pkcon -p remove " + package_id;
    execute_uninstall_command(command, callback);
}

void PackageManager::execute_uninstall_command(const std::string& command,
                                               std::function<void(int, std::string)> callback)
{
    QSharedPointer<QProcess> process(new QProcess());

    typedef void(QProcess::*QProcessFinished)(int, QProcess::ExitStatus);
    typedef void(QProcess::*QProcessError)(QProcess::ProcessError);
    QObject::connect(process.data(),
                     static_cast<QProcessFinished>(&QProcess::finished),
                     [process, callback](int code, QProcess::ExitStatus status) {
                         Q_UNUSED(status);
                         qDebug() << "command finished with exit code:" << code;
                         callback(code, process.data()->readAllStandardError().data());
                         if (code == 0) {
                             QProcess::execute(DBUSSEND_COMMAND);
                         }
                     } );
    QObject::connect(process.data(),
                     static_cast<QProcessError>(&QProcess::error),
                     [process, callback](QProcess::ProcessError error) {
                         qCritical() << "error running command:" << error;
                         callback(-255 + error, process.data()->readAllStandardError().data());
                     } );
    qDebug() << "Running command:" << command.c_str();
    process.data()->start(command.c_str());
}

Index::Index(const QSharedPointer<click::web::Client>& client,
             const QSharedPointer<Configuration> configuration) :
    client(client), configuration(configuration)
{

}

std::string Index::build_index_query(const std::string& query)
{
    std::stringstream result;

    result << query;
    return result.str();
}

std::map<std::string, std::string> Index::build_headers()
{
    std::stringstream frameworks;
    for (auto f: configuration->get_available_frameworks()) {
        frameworks << "," << f;
    }

    return std::map<std::string, std::string> {
        {"Accept", "application/hal+json"},
        {"X-Ubuntu-Frameworks", frameworks.str()},
        {"X-Ubuntu-Architecture", configuration->get_architecture()}
    };
}

click::web::Cancellable Index::search (const std::string& query, std::function<void(click::PackageList)> callback)
{
    click::web::CallParams params;
    const std::string built_query(build_index_query(query));
    params.add(click::QUERY_ARGNAME, built_query.c_str());
    QSharedPointer<click::web::Response> response(client->call(
        get_base_url() + click::SEARCH_PATH, "GET", false, build_headers(), "", params));

    QObject::connect(response.data(), &click::web::Response::finished, [=](QString reply) {
        Json::Reader reader;
        Json::Value root;

        click::PackageList pl;
        if (reader.parse(reply.toUtf8().constData(), root)) {
            pl = click::package_list_from_json_node(root);
            qDebug() << "found packages:" << pl.size();
        }
        callback(pl);
    });
    QObject::connect(response.data(), &click::web::Response::error, [=](QString /*description*/) {
        qDebug() << "No packages found due to network error";
        click::PackageList pl;
        qDebug() << "calling callback";
        callback(pl);
        qDebug() << "                ...Done!";
    });
    return click::web::Cancellable(response);
}

click::web::Cancellable Index::get_details (const std::string& package_name, std::function<void(PackageDetails, click::Index::Error)> callback)
{
    QSharedPointer<click::web::Response> response = client->call
        (get_base_url() + click::DETAILS_PATH + package_name);
    qDebug() << "getting details for" << package_name.c_str();

    QObject::connect(response.data(), &click::web::Response::finished, [=](const QByteArray reply) {
                    qDebug() << "index, response finished:" << reply.toPercentEncoding(" {},=:\n\"'");
                    click::PackageDetails d = click::PackageDetails::from_json(reply.constData());
                    qDebug() << "index, details title:" << QByteArray(d.package.title.c_str()).toPercentEncoding(" ");
                    callback(d, click::Index::Error::NoError);
                });
    QObject::connect(response.data(), &click::web::Response::error, [=](QString /*description*/) {
                    qDebug() << "Cannot get package details due to network error";
                    callback(PackageDetails(), click::Index::Error::NetworkError);
                });

    return click::web::Cancellable(response);
}

std::string Index::get_base_url ()
{
    const char *env_url = getenv(SEARCH_BASE_URL_ENVVAR.c_str());
    if (env_url != NULL) {
        return env_url;
    }
    return click::SEARCH_BASE_URL;
}

Index::~Index()
{
}

} // namespace click

#include "index.moc"
