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

#include <click/smartconnect.h>

#include <click/download-manager.h>
#include "index.h"
#include "interface.h"
#include "application.h"

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
                             invalidate_results(APPS_SCOPE_ID.toUtf8().data());
                             invalidate_results(STORE_SCOPE_ID.toUtf8().data());
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

void PackageManager::invalidate_results(const std::string& scope_id)
{
    QProcess::execute(REFRESH_SCOPE_COMMAND.arg(scope_id.c_str()));
}


Index::Index(const QSharedPointer<click::web::Client>& client,
             const QSharedPointer<Configuration> configuration) :
    client(client), configuration(configuration)
{

}

std::string Index::build_index_query(const std::string& query, const std::string& department)
{
    std::stringstream result;

    result << query;
    if (!department.empty()) {
        result << ",department:" << department;
    }

    return result.str();
}

std::map<std::string, std::string> Index::build_headers()
{
    std::stringstream frameworks;
    for (auto f: configuration->get_available_frameworks()) {
        frameworks << "," << f;
    }

    return std::map<std::string, std::string> {
        {"Accept", "application/hal+json,application/json"},
        {"X-Ubuntu-Frameworks", frameworks.str()},
        {"X-Ubuntu-Architecture", configuration->get_architecture()}
    };
}

std::pair<Packages, Packages> Index::package_lists_from_json(const std::string& json)
{
    Json::Reader reader;
    Json::Value root;

    click::Packages pl;
    click::Packages recommends;
    if (reader.parse(json, root)) {
        if (root.isObject() && root.isMember(Package::JsonKeys::embedded)) {
            auto const emb = root[Package::JsonKeys::embedded];
            if (emb.isObject() && emb.isMember(Package::JsonKeys::ci_package)) {
                auto const pkg = emb[Package::JsonKeys::ci_package];
                pl = click::package_list_from_json_node(pkg);

                if (emb.isMember(Package::JsonKeys::ci_recommends)) {
                    auto const rec = emb[Package::JsonKeys::ci_recommends];
                    recommends = click::package_list_from_json_node(rec);
                }
            }
        }
    }
    return std::pair<Packages, Packages>(pl, recommends);
}

click::web::Cancellable Index::search (const std::string& query, const std::string& department,
                                       std::function<void(click::Packages search_results, click::Packages recommendations)> callback)
{
    click::web::CallParams params;
    const std::string built_query(build_index_query(query, department));
    params.add(click::QUERY_ARGNAME, built_query.c_str());
    QSharedPointer<click::web::Response> response(client->call(
        get_base_url() + click::SEARCH_PATH, "GET", true, build_headers(), "", params));

    QObject::connect(response.data(), &click::web::Response::finished, [=](QString reply) {
            std::pair<Packages, Packages> package_lists;
            package_lists = package_lists_from_json(reply.toUtf8().constData());
            callback(package_lists.first, package_lists.second);
    });
    QObject::connect(response.data(), &click::web::Response::error, [=](QString /*description*/) {
        qDebug() << "No packages found due to network error";
        click::Packages pl;
        click::Packages recommends;
        qDebug() << "calling callback";
        callback(pl, recommends);
        qDebug() << "                ...Done!";
    });
    return click::web::Cancellable(response);
}

click::web::Cancellable Index::bootstrap(std::function<void(const click::DepartmentList&, const click::HighlightList&, Error, int)> callback)
{
    return departments(get_base_url() + click::BOOTSTRAP_PATH, callback);
}

click::web::Cancellable Index::departments(const std::string& department_href, std::function<void(const DepartmentList&, const HighlightList&, Error, int)> callback)
{
    click::web::CallParams params;
    QSharedPointer<click::web::Response> response(client->call(
        department_href, "GET", true, build_headers(), "", params));

    QObject::connect(response.data(), &click::web::Response::finished, [=](QString reply) {
            qDebug() << "departments request finished";
            Json::Reader reader;
            Json::Value root;

            // Get the suggested currency from the store.
            if (response->has_header(CURRENCY_HEADER)) {
                m_suggested_currency = response->get_header(CURRENCY_HEADER);
            } else {
                m_suggested_currency = Configuration::CURRENCY_DEFAULT;
            }

            click::DepartmentList depts;
            click::HighlightList highlights;
            if (reader.parse(reply.toUtf8().constData(), root)) {
                depts = Department::from_json_root_node(root);
                highlights = Highlight::from_json_root_node(root);
            }
            callback(depts, highlights, click::Index::Error::NoError, 0);
        });
    QObject::connect(response.data(), &click::web::Response::error, [=](QString /*description*/, int error_code) {
            qWarning() << "departments call failed due to network error";
            const click::DepartmentList depts;
            const click::HighlightList highlights;
            qDebug() << "departments: calling callback";
            callback(depts, highlights, click::Index::Error::NetworkError, error_code);
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

std::string Index::get_suggested_currency() const
{
    return m_suggested_currency;
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
