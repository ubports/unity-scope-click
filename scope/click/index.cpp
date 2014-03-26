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

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/exceptions.hpp>
#include <boost/foreach.hpp>
#include <sstream>

#include "download-manager.h"
#include "index.h"
#include "smartconnect.h"

namespace click
{

bool operator==(const Package& lhs, const Package& rhs) {
    return lhs.name == rhs.name &&
            lhs.title == rhs.title &&
            lhs.price == rhs.price &&
            lhs.icon_url == rhs.icon_url &&
            lhs.url == rhs.url;
}

bool operator==(const PackageDetails& lhs, const PackageDetails& rhs) {
    return lhs.name == rhs.name &&
            lhs.title == rhs.title &&
            lhs.icon_url == rhs.icon_url &&
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

    boost::property_tree::ptree pt;
    boost::property_tree::read_json(is, pt);

    PackageList pl;

    BOOST_FOREACH(boost::property_tree::ptree::value_type &v, pt)
    {
        assert(v.first.empty()); // array elements have no names
        auto node = v.second;
        Package p;
        p.name = node.get<std::string>("name");
        p.title = node.get<std::string>("title");
        p.price = node.get<std::string>("price");
        p.icon_url = node.get<std::string>("icon_url");
        p.url = node.get<std::string>("resource_url");
        pl.push_back(p);
    }
    return pl;
}

void PackageDetails::loadJson(const std::string &json)
{
    try
    {
        std::istringstream is(json);
        boost::property_tree::ptree node;
        boost::property_tree::read_json(is, node);

        // Mandatory details go here. That is, get<>(...) will throw as we
        // do not provide a default value if a value with the given key does not exist.
        name = node.get<std::string>(JsonKeys::name);
        title = node.get<std::string>(JsonKeys::title);
        icon_url = node.get<std::string>(JsonKeys::icon_url);
        description = node.get<std::string>(JsonKeys::description);
        download_url = node.get<std::string>(JsonKeys::download_url);
        license = node.get<std::string>(JsonKeys::license);

        // Optional details go here. That is, get<>(...) will *not* throw as we
        // provide a default value.
        rating = node.get<std::string>(JsonKeys::rating, "");
        keywords = node.get<std::string>(JsonKeys::keywords, "");
        terms_of_service = node.get<std::string>(JsonKeys::terms_of_service, "");
        publisher = node.get<std::string>(JsonKeys::publisher, "");
        main_screenshot_url = node.get<std::string>(JsonKeys::main_screenshot_url, "");

        try
        {
            auto more_scr_node = node.get_child(JsonKeys::more_screenshot_urls);
            BOOST_FOREACH(boost::property_tree::ptree::value_type &v, more_scr_node)
            {
                auto const scr = v.second.get<std::string>("");
                // more_screenshot_urls may contain main_screenshot_url, if so, skip it
                if (scr != main_screenshot_url)
                {
                    more_screenshots_urls.push_back(scr);
                }
            }
        }
        catch (boost::property_tree::ptree_bad_path const&)
        {
            // missing 'more_screenshots_urls', silently ignore
        }
        binary_filesize = node.get<std::string>(JsonKeys::binary_filesize, "");
        version = node.get<std::string>(JsonKeys::version, "");
        framework = node.get<std::string>(JsonKeys::framework, "");
    } catch(const std::exception& e)
    {
        std::cerr << "PackageDetails::loadJson: Exception thrown while decoding JSON: " << e.what() << std::endl;
    } catch(...)
    {
        std::cerr << "PackageDetails::loadJson: Exception thrown while decoding JSON." << std::endl;
    }
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
        << print_string_if_not_empty(details.name) << ", "
        << print_string_if_not_empty(details.title) << ", "
        << print_string_if_not_empty(details.icon_url) << ", "
        << print_string_if_not_empty(details.description) << ", "
        << print_string_if_not_empty(details.download_url) << ", "
        << print_string_if_not_empty(details.rating) << ", "
        << print_string_if_not_empty(details.keywords) << ", "
        << print_string_if_not_empty(details.terms_of_service) << ", "
        << print_string_if_not_empty(details.license) << ", "
        << print_string_if_not_empty(details.publisher) << ", "
        << print_string_if_not_empty(details.main_screenshot_url) << ", "
        << print_list_if_not_empty(details.more_screenshots_urls) << ", "
        << print_string_if_not_empty(details.binary_filesize) << ", "
        << print_string_if_not_empty(details.version) << ", "
        << print_string_if_not_empty(details.framework)
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

    sc->connect(response.data(), &click::web::Response::finished, [=](const QString& reply) {
                    click::PackageDetails d;
                    qDebug() << "response finished";
                    d.loadJson(reply.toUtf8().constData());
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
