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

#include "departments.h"
#include <iostream>

namespace click
{

/*Department::Department(const std::string& id, const std::string& name)
    : id_(id),
      name_(name),
      href_(""), //FIXME
      has_children_flag_(false)
{
}*/

Department::Department(const std::string& id, const std::string &name, const std::string& href, bool has_children)
    : id_(id),
      name_(name),
      href_(href),
      has_children_flag_(has_children)
{
}

std::string Department::id() const
{
    return id_;
}

std::string Department::name() const
{
    return name_;
}

std::string Department::href() const
{
    return href_;
}

bool Department::has_children_flag() const
{
    return has_children_flag_;
}

void Department::set_subdepartments(const std::list<Department::SPtr>& deps)
{
    sub_departments_ = deps;
}

std::list<Department::SPtr> Department::sub_departments() const
{
    return sub_departments_;
}

std::list<Department::SPtr> Department::from_json_node(const Json::Value& node)
{
    std::list<Department::SPtr> deps;

    for (uint i = 0; i < node.size(); i++)
    {
        auto const item = node[i];
        if (item.isObject() && item.isMember(Department::JsonKeys::name))
        {
            auto name = item[Department::JsonKeys::name].asString();
            const bool has_children = item.isMember(Department::JsonKeys::has_children) ? item[Department::JsonKeys::has_children].asBool() : false;

            auto const links = item[Department::JsonKeys::links];
            auto const self = links[Department::JsonKeys::self];
            auto const href = self[Department::JsonKeys::href].asString();

            auto dep = std::make_shared<Department>(name, name, href, has_children); //FIXME: id
            if (item.isObject() && item.isMember(Department::JsonKeys::embedded))
            {
                auto const emb = item[Department::JsonKeys::embedded];
                if (emb.isObject() && emb.isMember(Department::JsonKeys::department))
                {
                    auto const ditem = emb[Department::JsonKeys::department];
                    auto const subdeps = from_json_node(ditem);
                    dep->set_subdepartments(subdeps);
                }
            }
            deps.push_back(dep);
        }
    }

    return deps;
}

std::list<Department::SPtr> Department::from_json_root_node(const Json::Value& root)
{
    if (root.isObject() && root.isMember(Department::JsonKeys::embedded))
    {
        auto const emb = root[Department::JsonKeys::embedded];
        if (emb.isObject() && emb.isMember(Department::JsonKeys::department))
        {
            auto const ditem = emb[Department::JsonKeys::department];
            return from_json_node(ditem);
        }
    }

    return std::list<Department::SPtr>();
}

std::list<Department::SPtr> Department::from_json(const std::string& json)
{
    Json::Reader reader;
    Json::Value root;

    try
    {
        if (!reader.parse(json, root)) {
            throw std::runtime_error(reader.getFormattedErrorMessages());
        }

        if (root.isObject() && root.isMember(Department::JsonKeys::embedded))
        {
            auto const emb = root[Department::JsonKeys::embedded];
            if (emb.isObject() && emb.isMember(Department::JsonKeys::department))
            {
                auto const ditem = emb[Department::JsonKeys::department];
                return from_json_node(ditem);
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error parsing departments: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "Unknown error when parsing departments" << std::endl;
    }

    return std::list<Department::SPtr>();
}

}
