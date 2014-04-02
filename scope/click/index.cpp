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

#include <json/json.h>
#include <sstream>

#include "download-manager.h"
#include "index.h"
#include "application.h"
#include "smartconnect.h"

namespace json = Json;

namespace click
{

bool operator==(const Package& lhs, const Package& rhs) {
    return lhs.name == rhs.name &&
            lhs.title == rhs.title &&
            lhs.price == rhs.price &&
            lhs.icon_url == rhs.icon_url;
}

bool operator==(const PackageDetails& lhs, const PackageDetails& rhs) {
    return lhs.package == rhs.package &&
            lhs.description == rhs.description &&
            lhs.download_url == rhs.download_url &&
            lhs.rating == rhs.rating &&
            // TODO: keywords should be a list of strings
            lhs.keywords == rhs.keywords &&
            lhs.terms_of_service == rhs.terms_of_service &&
            lhs.license == rhs.license &&
            lhs.publisher == rhs.publisher &&
            lhs.main_screenshot_url == rhs.main_screenshot_url &&
            // TODO: more_screenshots_urls should be a list of strings
            lhs.more_screenshots_urls == rhs.more_screenshots_urls &&
            // TODO: binary_filesize should be a int/long
            lhs.binary_filesize == rhs.binary_filesize &&
            lhs.version == rhs.version &&
            lhs.framework == rhs.framework;
}

QDebug operator<< (QDebug d, const std::string &s) {
    d << QString::fromStdString(s);
    return d;
}

bool operator==(const Application& lhs, const Application& rhs) {
    return lhs.name == rhs.name &&
            lhs.title == rhs.title &&
            lhs.description == rhs.description &&
            lhs.main_screenshot == rhs.main_screenshot &&
            lhs.icon_url == rhs.icon_url;
}

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

PackageList package_list_from_json(const std::string& json)
{
    std::istringstream is(json);

    PackageList pl;

    json::Reader reader;
    json::Value root;

    if (!reader.parse(json, root)) {
        throw std::runtime_error(reader.getFormattedErrorMessages());
    }

    for (uint i = 0; i < root.size(); i++)
    {
        Package p;
        json::Value item = root[i];
        p.name = item[Package::JsonKeys::name].asString();
        p.title = item[Package::JsonKeys::title].asString();
        p.price = item[Package::JsonKeys::price].asDouble();
        p.icon_url = item[Package::JsonKeys::icon_url].asString();
        p.url = item[Package::JsonKeys::resource_url].asString();
        pl.push_back(p);
    }
    return pl;
}

PackageDetails PackageDetails::from_json(const std::string &json)
{
    PackageDetails details;
    try
    {
        json::Reader reader;
        json::Value root;

        if (!reader.parse(json, root))
            throw std::runtime_error(reader.getFormattedErrorMessages());

        // Mandatory details go here. That is, asString() will throw as we
        // do not provide a default value if a value with the given key does not exist.
        details.package.name = root[Package::JsonKeys::name].asString();
        details.package.title = root[Package::JsonKeys::title].asString();
        details.package.icon_url = root[Package::JsonKeys::icon_url].asString();
        details.package.price = root[Package::JsonKeys::price].asDouble();
        details.description = root[JsonKeys::description].asString();
        details.download_url = root[JsonKeys::download_url].asString();
        details.license = root[JsonKeys::license].asString();

        // Optional details go here.
        if (root[JsonKeys::version].isString())
            details.version = root[JsonKeys::version].asString();

        if (root[JsonKeys::rating].isNumeric())
            details.rating = root[JsonKeys::rating].asDouble();

        if (root[JsonKeys::keywords].isString())
            details.keywords = root[JsonKeys::keywords].asString();

        if (root[JsonKeys::terms_of_service].isString())
            details.terms_of_service = root[JsonKeys::terms_of_service].asString();

        if (root[JsonKeys::publisher].isString())
            details.publisher = root[JsonKeys::publisher].asString();

        if (root[JsonKeys::main_screenshot_url].isString())
            details.main_screenshot_url = root[JsonKeys::main_screenshot_url].asString();

        auto screenshots = root[JsonKeys::more_screenshot_urls];

        for (uint i = 0; i < screenshots.size(); i++)
        {
            auto scr = screenshots[i].asString();
            // more_screenshot_urls may contain main_screenshot_url, if so, skip it
            if (scr != details.main_screenshot_url)
            {
                details.more_screenshots_urls.push_back(scr);
            }
        }

        if (root[JsonKeys::binary_filesize].isIntegral())
            details.binary_filesize = root[JsonKeys::binary_filesize].asUInt64();

        if (root[JsonKeys::framework].isString())
            details.framework = root[JsonKeys::framework].asString();

    } catch(const std::exception& e)
    {
        std::cerr << "PackageDetails::loadJson: Exception thrown while decoding JSON: " << e.what() << std::endl;
    } catch(...)
    {
        std::cerr << "PackageDetails::loadJson: Exception thrown while decoding JSON." << std::endl;
    }

    return details;
}

std::string print_string_if_not_empty(const std::string& s)
{

    return s.empty() ? "n/a" : s;
}

std::string print_list_if_not_empty(const std::list<std::string>& li)
{
    std::stringstream s;
    s << "[";
    if (!li.empty())
    {
        auto it = li.begin();
        s << print_string_if_not_empty(*it);
        ++it;
        while (it != li.end())
        {
            s << ", " << print_string_if_not_empty(*it);
            ++it;
        }
    }
    s << "]";
    return s.str();
}

std::ostream& operator<<(std::ostream& out, const click::PackageDetails& details)
{
    out << "("
        << print_string_if_not_empty(details.package.name) << ", "
        << print_string_if_not_empty(details.package.title) << ", "
        << print_string_if_not_empty(details.package.icon_url) << ", "
        << print_string_if_not_empty(details.description) << ", "
        << print_string_if_not_empty(details.download_url) << ", "
        << details.rating << ", "
        << print_string_if_not_empty(details.keywords) << ", "
        << print_string_if_not_empty(details.terms_of_service) << ", "
        << print_string_if_not_empty(details.license) << ", "
        << print_string_if_not_empty(details.publisher) << ", "
        << print_string_if_not_empty(details.main_screenshot_url) << ", "
        << print_list_if_not_empty(details.more_screenshots_urls) << ", "
        << details.binary_filesize << ", "
        << print_string_if_not_empty(details.version) << ", "
        << print_string_if_not_empty(details.framework)
        << ")";

    return out;
}

std::ostream& operator<<(std::ostream& out, const click::Application& app)
{
    out << "("
        << print_string_if_not_empty(app.name) << ", "
        << print_string_if_not_empty(app.title) << ", "
        << app.price << ", "
        << print_string_if_not_empty(app.icon_url) << ", "
        << print_string_if_not_empty(app.url) << ", "
        << print_string_if_not_empty(app.version) << ", "
        << print_string_if_not_empty(app.description) << ", "
        << print_string_if_not_empty(app.main_screenshot)
        << ")";

    return out;
}

Index::Index(const QSharedPointer<click::web::Client>& client) : client(client)
{

}

click::web::Cancellable Index::search (const std::string& query, std::function<void(click::PackageList)> callback)
{
    click::web::CallParams params;
    params.add(click::QUERY_ARGNAME, query.c_str());
    QSharedPointer<click::web::Response> response(client->call(
        click::SEARCH_BASE_URL + click::SEARCH_PATH, params));

    auto sc = new click::utils::SmartConnect();
    response->setParent(sc);
    sc->connect(response.data(), &click::web::Response::finished, [=](QString reply) {
        click::PackageList pl = click::package_list_from_json(reply.toUtf8().constData());
        callback(pl);
    });
    sc->connect(response.data(), &click::web::Response::error, [=](QString /*description*/) {
        qDebug() << "No packages found due to network error";
        click::PackageList pl;
        callback(pl);
    });
    return click::web::Cancellable(response);
}

click::web::Cancellable Index::get_details (const std::string& package_name, std::function<void(PackageDetails, click::Index::Error)> callback)
{
    QSharedPointer<click::web::Response> response = client->call
        (click::SEARCH_BASE_URL + click::DETAILS_PATH + package_name);
    qDebug() << "getting details for" << package_name.c_str();

    auto sc = new click::utils::SmartConnect();
    response->setParent(sc);

    sc->connect(response.data(), &click::web::Response::finished, [=](const QByteArray reply) {
                    qDebug() << "index, response finished:" << reply.toPercentEncoding(" {},=:\n\"'");
                    click::PackageDetails d = click::PackageDetails::from_json(reply.constData());
                    qDebug() << "index, details title:" << QByteArray(d.package.title.c_str()).toPercentEncoding(" ");
                    callback(d, click::Index::Error::NoError);
                });
    sc->connect(response.data(), &click::web::Response::error, [=](QString /*description*/) {
                    qDebug() << "Cannot get package details due to network error";
                    callback(PackageDetails(), click::Index::Error::NetworkError);
                });

    return click::web::Cancellable(response);
}

Index::~Index()
{
}

} // namespace click

#include "index.moc"
