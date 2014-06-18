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
#include "store-query.h"
#include <click/qtbridge.h>

#include <click/key_file_locator.h>

#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CannedQuery.h>
#include <unity/scopes/SearchReply.h>
#include <unity/scopes/SearchMetadata.h>

#include<vector>
#include<set>
#include<sstream>
#include <cassert>

#include <click/click-i18n.h>

using namespace click;

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
            "subtitle": "subtitle",
            "art" : {
                "field": "art",
                "aspect-ratio": 1.13
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
            "subtitle": "subtitle"
        }
    }
)";

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
    click::web::Cancellable search_operation;
};

click::Query::Query(unity::scopes::CannedQuery const& query, click::Index& index, scopes::SearchMetadata const& metadata)
    : impl(new Private(query, index, metadata))
{
}

click::Query::~Query()
{
    qDebug() << "destroying search";
}

void click::Query::cancelled()
{
    qDebug() << "cancelling search of" << QString::fromStdString(impl->query.query_string());
    impl->search_operation.cancel();
}

click::Interface& click::Query::clickInterfaceInstance()
{
    static QSharedPointer<click::KeyFileLocator> keyFileLocator(new click::KeyFileLocator());
    static click::Interface iface(keyFileLocator);

    return iface;
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
                                      const PackageSet& installedPackages,
                                      const std::string& categoryTemplate)
{
    scopes::CategoryRenderer categoryRenderer(categoryTemplate);
    auto category = register_category(searchReply, "appstore", _("Available"), "", categoryRenderer);

    scopes::CategoryRenderer recommendsCatRenderer(categoryTemplate);
    auto recommendsCategory = register_category(searchReply, "recommends",
                                                _("Recommended"), "",
                                                recommendsCatRenderer);

    run_under_qt([=]()
    {
        auto search_cb = [this, searchReply,
                          category, recommendsCategory,
                          installedPackages](Packages packages, Packages recommends) {
                qDebug("search callback");

                // handle packages data
                foreach (auto p, packages) {
                    qDebug() << "pushing result" << QString::fromStdString(p.name);
                    try {
                        scopes::CategorisedResult res(category);
                        res.set_title(p.title);
                        res.set_art(p.icon_url);
                        res.set_uri(p.url);
                        res[click::Query::ResultKeys::NAME] = p.name;
                        auto installed = installedPackages.find(p);
                        if (installed != installedPackages.end()) {
                            res[click::Query::ResultKeys::INSTALLED] = true;
                            res["subtitle"] = _("✔ INSTALLED");
                            res[click::Query::ResultKeys::VERSION] = installed->version;
                        } else {
                            res[click::Query::ResultKeys::INSTALLED] = false;
                            // TODO: get the real price from the webservice (upcoming branch)
                            res["subtitle"] = _("FREE");
                        }

                        this->push_result(searchReply, res);
                    } catch(const std::exception& e){
                        qDebug() << "PackageDetails::loadJson: Exception thrown while decoding JSON: " << e.what() ;
                    } catch(...){
                        qDebug() << "no reason to catch";
                    }
                }
                foreach (auto r, recommends) {
                    try {
                        scopes::CategorisedResult res(recommendsCategory);
                        res.set_title(r.title);
                        res.set_art(r.icon_url);
                        res.set_uri(r.url);
                        res[click::Query::ResultKeys::NAME] = r.name;
                        if (r.price == 0.00) {
                            res["subtitle"] = _("FREE");
                        } else {
                            // TODO: Show the correctly formatted price
                        }

                        this->push_result(searchReply, res);
                    } catch (...) {
                        qWarning() << "Failed to load recommendation:" << r.name.c_str();
                    }
                }
                qDebug() << "search completed";
                this->finished(searchReply);
        };

            qDebug() << "starting search of" << QString::fromStdString(impl->query.query_string());
            impl->search_operation = impl->index.search(impl->query.query_string(), search_cb);
    });
}

PackageSet click::Query::get_installed_packages()
{
    std::promise<PackageSet> installed_promise;
    std::future<PackageSet> installed_future = installed_promise.get_future();

    run_under_qt([&]()
    {
        clickInterfaceInstance().get_installed_packages(
                    [&installed_promise](PackageSet installedPackages, InterfaceError){
            installed_promise.set_value(installedPackages);
        });
    });

    return installed_future.get();
}


void click::Query::run(scopes::SearchReplyProxy const& searchReply)
{
    auto query = impl->query.query_string();
    std::string categoryTemplate = CATEGORY_APPS_SEARCH;
    if (query.empty()) {
        categoryTemplate = CATEGORY_APPS_DISPLAY;
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

    add_available_apps(searchReply, get_installed_packages(), categoryTemplate);
}
