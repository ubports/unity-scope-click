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

#include <click/click-i18n.h>
#include "highlights.h"
#include <iostream>

namespace click
{

Highlight::Highlight(const std::string& name)
    : name_(name)
{
}

Highlight::Highlight(const std::string& slug, const std::string& name, const Packages& pkgs, bool contains_scopes)
    : slug_(slug),
      name_(name),
      packages_(pkgs),
      contains_scopes_(contains_scopes)
{
}

void Highlight::add_package(const Package& pkg)
{
    packages_.push_back(pkg);
}

bool Highlight::contains_scopes() const
{
    return contains_scopes_;
}

std::string Highlight::slug() const
{
    return slug_;
}

std::string Highlight::name() const
{
    return name_;
}

Packages Highlight::packages() const
{
    return packages_;
}

std::list<Highlight> Highlight::from_json_node(const Json::Value& node)
{
    std::list<Highlight> highlights;

    for (uint i = 0; i < node.size(); i++)
    {
        auto const item = node[i];
        if (item.isObject() && item.isMember(Highlight::JsonKeys::name))
        {
            auto name = item[Highlight::JsonKeys::name].asString();
            auto slug = item[Highlight::JsonKeys::slug].asString();
            auto pkg_node = item[Package::JsonKeys::embedded][Package::JsonKeys::ci_package];
            auto pkgs = package_list_from_json_node(pkg_node);
            auto hl = Highlight(slug, name, pkgs);
            if (slug == "app-of-the-week" || slug == "editors-pick") {
                highlights.push_front(hl);
            } else {
                highlights.push_back(hl);
            }
        }
    }

    return highlights;
}

std::list<Highlight> Highlight::from_json_root_node(const Json::Value& root)
{
    std::list<Highlight> highlights;
    if (root.isObject() && root.isMember(Highlight::JsonKeys::embedded))
    {
        auto const emb = root[Highlight::JsonKeys::embedded];
        if (emb.isObject() && emb.isMember(Highlight::JsonKeys::highlight))
        {
            auto const hl = emb[Highlight::JsonKeys::highlight];
            highlights = from_json_node(hl);
        }
        if (emb.isObject() && emb.isMember(Package::JsonKeys::ci_package))
        {
            auto pkg_node = emb[Package::JsonKeys::ci_package];
            auto pkgs = package_list_from_json_node(pkg_node);
            click::Packages apps;
            click::Packages scopes;

            for (click::Package& p : pkgs) {
                if (p.content == "scope") {
                    scopes.push_back(p);
                } else {
                    apps.push_back(p);
                }
            }

            if (scopes.size() > 0) {
                highlights.push_back(Highlight("__all-scopes__", _("Scopes"), scopes, true));
            }

            if (apps.size() > 0) {
                highlights.push_back(Highlight("__all-apps__", _("Apps"), apps));
            }
        }
    }

    return highlights;
}

}
