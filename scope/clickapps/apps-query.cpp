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

#include <click/application.h>
#include <click/interface.h>

#include <click/key_file_locator.h>

#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CannedQuery.h>
#include <unity/scopes/SearchReply.h>
#include <unity/scopes/SearchMetadata.h>

#include <vector>
#include <algorithm>

#include <click/click-i18n.h>
#include "apps-query.h"

namespace
{

std::string CATEGORY_APPS_DISPLAY = R"(
    {
        "schema-version" : 1,
        "template" : {
            "category-layout" : "grid",
            "card-size": "small"
        },
        "components" : {
            "title" : "title",
            "art" : {
                "field": "art",
                "aspect-ratio": 1.6,
                "fill-mode": "fit"
            }
        }
    }
)";

std::string CATEGORY_APPS_SEARCH = R"(
    {
        "schema-version" : 1,
        "template" : {
            "category-layout" : "grid",
            "card-layout" : "horizontal",
            "card-size": "large"
        },
        "components" : {
            "title" : "title",
            "mascot" : {
                "field": "art"
            },
            "subtitle": "publisher"
        }
    }
)";

}

void click::Query::push_local_results(scopes::SearchReplyProxy const &replyProxy,
                                      std::vector<click::Application> const &apps,
                                      std::string &categoryTemplate)
{
    scopes::CategoryRenderer rdr(categoryTemplate);
    auto cat = replyProxy->register_category("local", "", "", rdr);

    for(const auto & a: apps)
    {
        scopes::CategorisedResult res(cat);
        res.set_title(a.title);
        res.set_art(a.icon_url);
        res.set_uri(a.url);
        res[click::Query::ResultKeys::NAME] = a.name;
        res[click::Query::ResultKeys::DESCRIPTION] = a.description;
        res[click::Query::ResultKeys::MAIN_SCREENSHOT] = a.main_screenshot;
        res[click::Query::ResultKeys::INSTALLED] = true;
        res[click::Query::ResultKeys::VERSION] = a.version;
        replyProxy->push(res);
    }
}

struct click::Query::Private
{
    Private(const unity::scopes::CannedQuery& query, click::Index& index, const scopes::SearchMetadata& metadata)
        : query(query),
          index(index),
          meta(metadata)
    {
    }
    unity::scopes::CannedQuery query;
    click::Index& index;
    scopes::SearchMetadata meta;
};

click::Query::Query(unity::scopes::CannedQuery const& query, click::Index& index, scopes::SearchMetadata const& metadata)
    : impl(new Private(query, index, metadata))
{
}

void click::Query::cancelled()
{
    qDebug() << "cancelling search of" << QString::fromStdString(impl->query.query_string());
}

click::Query::~Query()
{
    qDebug() << "destroying search";
}

namespace
{
click::Interface& clickInterfaceInstance()
{
    static QSharedPointer<click::KeyFileLocator> keyFileLocator(new click::KeyFileLocator());
    static click::Interface iface(keyFileLocator);

    return iface;
}

}

void click::Query::add_fake_store_app(std::vector<click::Application>& apps)
{
    static const unity::scopes::CannedQuery store_scope("com.canonical.scopes.clickstore");
    click::Application store;
    store.name = store.title = _("Ubuntu Store");

    std::string name = store.name;
    std::string query = impl->query.query_string();
    std::transform(query.begin(), query.end(), query.begin(), ::tolower);
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    if (query.empty() || name.find(query) != std::string::npos)
    {
        store.url = store_scope.to_uri();
        store.icon_url = "/usr/share/icons/ubuntu-mobile/apps/scalable/ubuntuone.svg";
        store.installed_time = 0;
        apps.push_back(store);
    }
}

void click::Query::run(scopes::SearchReplyProxy const& searchReply)
{
    auto query = impl->query.query_string();
    std::string categoryTemplate = CATEGORY_APPS_SEARCH;
    if (query.empty()) {
        categoryTemplate = CATEGORY_APPS_DISPLAY;
    }
    auto localResults = clickInterfaceInstance().find_installed_apps(
                query);

    add_fake_store_app(localResults);

    // Sort applications so that newest come first.
    std::sort(localResults.begin(), localResults.end(), [](const Application& a, const Application& b) {
                  return a.installed_time > b.installed_time;
                });

    push_local_results(
        searchReply,
        localResults,
        categoryTemplate);
}
