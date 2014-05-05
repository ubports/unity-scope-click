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

#include "application.h"
#include "query.h"
#include "qtbridge.h"
#include "key_file_locator.h"
#include "interface.h"

#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CannedQuery.h>
#include <unity/scopes/SearchReply.h>
#include <unity/scopes/SearchMetadata.h>

#include<vector>
#include<set>
#include<sstream>

#include "click-i18n.h"

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
    auto cat = replyProxy->register_category("local", _("My apps"), "", rdr);

    // cat might be null when the underlying query got cancelled.
    if (!cat)
        return;

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
    Private(const std::string& query, click::Index& index, const scopes::SearchMetadata& metadata)
        : query(query),
          index(index),
          meta(metadata)
    {
    }
    std::string query;
    click::Index& index;
    scopes::SearchMetadata meta;
    click::web::Cancellable search_operation;
};

click::Query::Query(std::string const& query, click::Index& index, scopes::SearchMetadata const& metadata)
    : impl(new Private(query, index, metadata))
{
}

click::Query::~Query()
{
    qDebug() << "destroying search";
}

void click::Query::cancelled()
{
    qDebug() << "cancelling search of" << QString::fromStdString(impl->query);
    impl->search_operation.cancel();
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

bool click::Query::push_result(scopes::SearchReplyProxy const& searchReply, const scopes::CategorisedResult &res)
{
    return searchReply->push(res);
}

void click::Query::finished(const scopes::SearchReplyProxy &searchReply)
{
    searchReply->finished();
}

scopes::Category::SCPtr click::Query::register_category(const scopes::SearchReplyProxy &searchReply,
                                                        const std::string &id,
                                                        const std::string &title,
                                                        const std::string &icon,
                                                        const scopes::CategoryRenderer &renderer_template)
{
    return searchReply->register_category(id, title, icon, renderer_template);
}

void click::Query::run_under_qt(const std::function<void ()> &task)
{
    qt::core::world::enter_with_task([task]() {
        task();
    });
}

void click::Query::add_available_apps(scopes::SearchReplyProxy const& searchReply,
                                      const std::set<std::string>& locallyInstalledApps,
                                      const std::string& categoryTemplate)
{
    scopes::CategoryRenderer categoryRenderer(categoryTemplate);
    auto category = register_category(searchReply, "appstore", _("Available"), "", categoryRenderer);
    if (!category) {
        // category might be null when the underlying query got cancelled.
        qDebug() << "category is null";
        return;
    }

    run_under_qt([=]()
    {
        qDebug() << "starting search of" << QString::fromStdString(impl->query);

        impl->search_operation = impl->index.search(impl->query, [=](PackageList packages){
            qDebug("callback here");
            foreach (auto p, packages) {
                qDebug() << "pushing result" << QString::fromStdString(p.name);
                try {
                    scopes::CategorisedResult res(category);
                    if (locallyInstalledApps.count(p.name) > 0) {
                        qDebug() << "already installed" << QString::fromStdString(p.name);
                        continue;
                    }
                    res.set_title(p.title);
                    res.set_art(p.icon_url);
                    res.set_uri(p.url);
                    res[click::Query::ResultKeys::NAME] = p.name;
                    res[click::Query::ResultKeys::INSTALLED] = false;

                    this->push_result(searchReply, res);
                } catch(const std::exception& e){
                    qDebug() << "PackageDetails::loadJson: Exception thrown while decoding JSON: " << e.what() ;
                } catch(...){
                    qDebug() << "no reason to catch";
                }
            }
            qDebug() << "search completed";
            this->finished(searchReply);
        });

    });

}

void click::Query::run(scopes::SearchReplyProxy const& searchReply)
{
    std::string categoryTemplate = CATEGORY_APPS_SEARCH;
    if (impl->query.empty()) {
        categoryTemplate = CATEGORY_APPS_DISPLAY;
    }
    auto localResults = clickInterfaceInstance().find_installed_apps(
                impl->query);

    push_local_results(
        searchReply, 
        localResults,
        categoryTemplate);

    std::set<std::string> locallyInstalledApps;
    for(const auto& app : localResults) {
        locallyInstalledApps.insert(app.name);
    }

    static const std::string no_net_hint("no-internet");
    if (impl->meta.contains_hint(no_net_hint))
    {
        auto var = impl->meta[no_net_hint];
        if (var.which() == scopes::Variant::Type::Bool && var.get_bool())
        {
            return;
        }

    }

    add_available_apps(searchReply, locallyInstalledApps, categoryTemplate);
}
