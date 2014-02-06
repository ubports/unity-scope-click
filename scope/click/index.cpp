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

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>

#include "index.h"

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
            lhs.keywords == rhs.keywords &&
            lhs.terms_of_service == rhs.terms_of_service &&
            lhs.license == rhs.license &&
            lhs.publisher == rhs.publisher &&
            lhs.main_screenshot_url == rhs.main_screenshot_url &&
            lhs.more_screenshots_urls == rhs.more_screenshots_urls &&
            lhs.binary_filesize == rhs.binary_filesize &&
            lhs.version == rhs.version &&
            lhs.framework == rhs.framework;
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
    std::istringstream is(json);
    boost::property_tree::ptree node;
    boost::property_tree::read_json(is, node);
    name = node.get<std::string>("name");
    title = node.get<std::string>("title");
    icon_url = node.get<std::string>("icon_url");
    description = node.get<std::string>("description");
    download_url = node.get<std::string>("download_url");
    rating = node.get<std::string>("rating");
    keywords = node.get<std::string>("keywords");
    terms_of_service = node.get<std::string>("terms_of_service");
    license = node.get<std::string>("license");
    publisher = node.get<std::string>("publisher");
    main_screenshot_url = node.get<std::string>("screenshot_url");
    more_screenshots_urls = node.get<std::string>("screenshot_urls");
    binary_filesize = node.get<std::string>("binary_filesize");
    version = node.get<std::string>("version");
    framework = node.get<std::string>("framework");
}

Index::Index(const QSharedPointer<click::web::Service>& service) : service(service)
{

}

void Index::search (const std::string& query, std::function<void(click::PackageList)> callback)
{
    click::web::CallParams params;
    params.add(click::QUERY_ARGNAME, query.c_str());
    QSharedPointer<click::web::Response> response(service->call(click::SEARCH_PATH, params));
    QObject::connect(response.data(), &click::web::Response::finished, [=](QString reply) {
        Q_UNUSED(response); // so it's still in scope
        click::PackageList pl = click::package_list_from_json(reply.toUtf8().constData());
        callback(pl);
    });
}

void Index::get_details (const std::string& package_name, std::function<void(PackageDetails)> callback)
{
    QSharedPointer<click::web::Response> response = service->call(click::DETAILS_PATH+package_name);
    QObject::connect(response.data(), &click::web::Response::finished, [=](QString reply){
        Q_UNUSED(response); // so it's still in scope
        click::PackageDetails d;
        d.loadJson(reply.toUtf8().constData());
        callback(d);
    });
}

Index::~Index()
{
}

} // namespace click
