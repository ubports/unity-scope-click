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

#ifndef CLICK_DEPARTMENTS_H
#define CLICK_DEPARTMENTS_H

#include <string>
#include <list>
#include <memory>
#include <json/json.h>

namespace click
{

class Department
{
    public:
        typedef std::shared_ptr<Department> SPtr;
        typedef std::shared_ptr<Department const> SCPtr;

        struct JsonKeys
        {
            JsonKeys() = delete;
            constexpr static const char* name {"name"};
            constexpr static const char* embedded {"_embedded"};
            constexpr static const char* department {"clickindex:department"};
            constexpr static const char* has_children {"has_children"};
        };

        Department(const std::string &id, const std::string &name);
        Department(const std::string &id, const std::string &name, bool has_children);
        std::string id() const;
        std::string name() const;
        std::string href() const;
        bool has_children_flag() const;
        void set_subdepartments(const std::list<Department::SPtr>& deps);
        std::list<Department::SPtr> sub_departments() const;
        static std::list<Department::SPtr> from_json(const std::string& json);
        static std::list<Department::SPtr> from_json_root_node(const Json::Value& val);

    private:
        static std::list<Department::SPtr> from_json_node(const Json::Value& val);
        std::string id_;
        std::string name_;
        std::string href_;
        bool has_children_flag_;
        std::list<Department::SPtr> sub_departments_;
};

typedef std::list<Department::SPtr> DepartmentList;

}

#endif
