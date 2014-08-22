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

#ifndef CLICK_HIGHLIGHTS_H
#define CLICK_HIGHLIGHTS_H

#include <string>
#include <list>
#include <json/json.h>
#include <click/package.h>

namespace click
{

class Highlight
{
public:
    struct JsonKeys
    {
        JsonKeys() = delete;
        constexpr static const char* name {"name"};
        constexpr static const char* slug {"slug"};
        constexpr static const char* embedded {"_embedded"};
        constexpr static const char* highlight {"clickindex:highlight"};
    };

    Highlight(const std::string& name);
    Highlight(const std::string& slug, const std::string& name, const Packages& pkgs, bool contains_scopes=false);
    void add_package(const Package& pkg);

    std::string name() const;
    std::string slug() const;
    Packages packages() const;
    bool contains_scopes() const;

    static std::list<Highlight> from_json_root_node(const Json::Value& val);

private:
    static std::list<Highlight> from_json_node(const Json::Value& val);

    std::string slug_;
    std::string name_;
    Packages packages_;
    bool contains_scopes_;
};

typedef std::list<Highlight> HighlightList;

}

#endif
