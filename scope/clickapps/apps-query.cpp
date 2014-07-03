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

static const char CATEGORY_STORE[] = R"(
{
  "template": {
    "category-layout": "grid",
    "overlay": true,
    "card-size": "small",
    "card-background": "color:///#DD4814"
  },
  "components": {
    "title": "title",
    "art": {
      "aspect-ratio": 0.55,
      "field": "art"
    }
  }
}

)";


}

void click::apps::ResultPusher::push_result(scopes::Category::SCPtr& cat, const click::Application& a)
{
    scopes::CategorisedResult res(cat);
    res.set_title(a.title);
    res.set_art(a.icon_url);
    res.set_uri(a.url);
    res[click::apps::Query::ResultKeys::NAME] = a.name;
    res[click::apps::Query::ResultKeys::DESCRIPTION] = a.description;
    res[click::apps::Query::ResultKeys::MAIN_SCREENSHOT] = a.main_screenshot;
    res[click::apps::Query::ResultKeys::INSTALLED] = true;
    res[click::apps::Query::ResultKeys::VERSION] = a.version;
    replyProxy->push(res);
}


void click::apps::ResultPusher::push_local_results(
                                      std::vector<click::Application> const &apps,
                                      std::string &categoryTemplate)
{
    scopes::CategoryRenderer rdr(categoryTemplate);
    auto cat = replyProxy->register_category("local", _("My apps"), "", rdr);

    for(const auto & a: apps)
    {
        push_result(cat, a);
    }
}

struct click::apps::Query::Private
{
    Private(const scopes::SearchMetadata& metadata)
        : meta(metadata)
    {
    }
    scopes::SearchMetadata meta;
    click::Configuration configuration;
};

click::apps::Query::Query(unity::scopes::CannedQuery const& query, scopes::SearchMetadata const& metadata)
    : unity::scopes::SearchQueryBase(query, metadata),
      impl(new Private(metadata))
{
}

void click::apps::Query::cancelled()
{
    qDebug() << "cancelling search of" << QString::fromStdString(query().query_string());
}

click::apps::Query::~Query()
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

void click::apps::Query::add_fake_store_app(scopes::SearchReplyProxy const& searchReply)
{
    static const std::string title = _("Ubuntu Store");
    static const std::string cat_title = _("Get more apps from the store");
    auto name = title;

    std::string querystr = query().query_string();
    std::transform(querystr.begin(), querystr.end(), querystr.begin(), ::tolower);
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    if (querystr.empty() || name.find(querystr) != std::string::npos)
    {
        scopes::CategoryRenderer rdr(CATEGORY_STORE);
        auto cat = searchReply->register_category("store", cat_title, "", rdr);

        static const unity::scopes::CannedQuery store_scope("com.canonical.scopes.clickstore");

        scopes::CategorisedResult res(cat);
        res.set_title(title);
        res.set_art(STORE_DATA_DIR "/store-scope-icon.svg");
        res.set_uri(store_scope.to_uri());
        res[click::apps::Query::ResultKeys::NAME] = title;
        res[click::apps::Query::ResultKeys::DESCRIPTION] = "";
        res[click::apps::Query::ResultKeys::MAIN_SCREENSHOT] = "";
        res[click::apps::Query::ResultKeys::INSTALLED] = true;
        res[click::apps::Query::ResultKeys::VERSION] = "";
        searchReply->push(res);
    }
}

std::vector<click::Application> click::apps::ResultPusher::push_top_results(
        scopes::SearchReplyProxy replyProxy,
        std::vector<click::Application> apps,
        std::string& categoryTemplate)
{
    Q_UNUSED(replyProxy)
    Q_UNUSED(categoryTemplate)
    return apps;
}

void click::apps::Query::run(scopes::SearchReplyProxy const& searchReply)
{
    auto querystr = query().query_string();
    ResultPusher pusher(searchReply, impl->configuration);
    std::string categoryTemplate = CATEGORY_APPS_SEARCH;
    auto localResults = clickInterfaceInstance().find_installed_apps(
                querystr);

    if (querystr.empty()) {
        categoryTemplate = CATEGORY_APPS_DISPLAY;
//        localResults = pusher.push_top_results(
//                    searchReply, localResults,
//                    categoryTemplate);
    }

    pusher.push_local_results(
        localResults,
        categoryTemplate);

    add_fake_store_app(searchReply);
}
