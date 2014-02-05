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

Index::Index(const QSharedPointer<click::web::Service>& service) : service(service)
{

}

void Index::search (const std::string& query, std::function<void(click::PackageList)> callback)
{
    click::web::CallParams params;
    params.add(click::QUERY_ARGNAME, query.c_str());
    QSharedPointer<click::web::Response> response(service->call(click::SEARCH_PATH, params));
    QObject::connect(response.data(), &click::web::Response::finished, [=](QString reply){
        click::PackageList pl = click::package_list_from_json(reply.toUtf8().constData());
        callback(pl);
    });
}

Index::~Index()
{
}

} // namespace click
