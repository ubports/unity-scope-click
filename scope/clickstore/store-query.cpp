/*
 * Copyright (C) 2014-2015 Canonical Ltd.
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

#include "store-query.h"
#include "store-scope.h"

#include <click/application.h>
#include <click/interface.h>
#include <click/key_file_locator.h>
#include <click/qtbridge.h>
#include <click/departments-db.h>

#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/Department.h>
#include <unity/scopes/CannedQuery.h>
#include <unity/scopes/SearchReply.h>
#include <unity/scopes/SearchMetadata.h>
#include <unity/scopes/Variant.h>
#include <unity/scopes/VariantBuilder.h>

#include <iostream>
#include <iomanip>
#include<vector>
#include<set>
#include<sstream>
#include <cassert>
#include <locale>

#include <QLocale>

#include <click/click-i18n.h>

using namespace click;

namespace
{

static const std::string CATEGORY_APPS_DISPLAY = R"(
    {
        "schema-version" : 1,
        "template" : {
            "category-layout" : "grid",
            "card-size": "small"
        },
        "components" : {
            "title" : "title",
            "subtitle": "subtitle",
            "attributes": { "field": "attributes", "max-count": 4 },
            "art" : {
                "field": "art",
                "aspect-ratio": 1.13,
                "fallback": "image://theme/placeholder-app-icon"
            }
        }
    }
)";

static const std::string CATEGORY_SCOPES_DISPLAY = R"(
    {
        "schema-version" : 1,
        "template" : {
            "overlay": true,
            "category-layout" : "grid",
            "card-size": "small"
        },
        "components" : {
            "title" : "title",
            "attributes": { "field": "attributes", "max-count": 1 },
            "art" : {
                "field": "art",
                "aspect-ratio": 0.55,
                "fallback": "image://theme/placeholder-app-icon"
            }
        }
    }
)";

static const std::string CATEGORY_APP_OF_THE_WEEK = R"(
{
    "schema-version" : 1,
    "template": {
        "category-layout": "grid",
        "card-size": "large"
    },
    "components": {
        "title": "title",
        "subtitle": "subtitle",
        "attributes": { "field": "attributes", "max-count": 4 },
        "art": {
            "aspect-ratio": 2.5,
            "field": "art",
            "fallback": "image://theme/placeholder-app-icon"
        }
    }
})";

static const std::string CATEGORY_APPS_SEARCH = R"(
    {
        "schema-version" : 1,
        "template" : {
            "category-layout" : "grid",
            "card-layout" : "horizontal",
            "collapsed-rows": 0,
            "card-size": "large"
        },
        "components" : {
            "title" : "title",
            "art" : {
                "field": "art",
                "aspect-ratio": 1.13,
                "fallback": "image://theme/placeholder-app-icon"
            },
            "subtitle": "subtitle",
            "attributes": { "field": "attributes", "max-count": 3 }
        }
    }
)";

}

struct click::Query::Private
{
    Private(click::Index& index, click::DepartmentLookup& depts,
            std::shared_ptr<click::DepartmentsDb> depts_db,
            click::HighlightList& highlights,
            const scopes::SearchMetadata& metadata,
            pay::Package& in_package)
        : index(index),
          department_lookup(depts),
          depts_db(depts_db),
          highlights(highlights),
          meta(metadata),
          pay_package(in_package)
    {
    }
    click::Index& index;
    click::DepartmentLookup& department_lookup;
    std::shared_ptr<click::DepartmentsDb> depts_db;
    click::HighlightList& highlights;
    scopes::SearchMetadata meta;
    click::web::Cancellable search_operation;
    click::web::Cancellable purchases_operation;
    pay::Package& pay_package;
};

click::Query::Query(unity::scopes::CannedQuery const& query,
                    click::Index& index,
                    click::DepartmentLookup& depts,
                    std::shared_ptr<click::DepartmentsDb> depts_db,
                    click::HighlightList& highlights,
                    scopes::SearchMetadata const& metadata,
                    pay::Package& in_package)
    : unity::scopes::SearchQueryBase(query, metadata),
      impl(new Private(index, depts, depts_db, highlights, metadata, in_package))
{
}

click::Query::~Query()
{
    qDebug() << "destroying search";
}

void click::Query::cancelled()
{
    qDebug() << "cancelling search of" << QString::fromStdString(query().query_string());
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

unity::scopes::Department::SPtr click::Query::fromClickDepartment(const click::Department::SCPtr click_dept, const std::string& current_dept_id, const click::DepartmentList& subdepts)
{
    const std::locale loc("");
    unity::scopes::Department::SPtr dep = unity::scopes::Department::create(click_dept->id(), query(), click_dept->name());
    if (click_dept->has_children_flag())
    {
        dep->set_has_subdepartments();
    }
    unity::scopes::DepartmentList departments;

    // subdepts is a list of subdepartments of current department fetched from the Store for current search; if we encountered current department in the tree,
    // insert the list from the server rather than the one from internal cache.
    for (auto click_subdep: (click_dept->id() == current_dept_id ? subdepts : click_dept->sub_departments()))
    {
        departments.push_back(fromClickDepartment(click_subdep, current_dept_id, subdepts));
    }
    departments.sort([&loc](const unity::scopes::Department::SCPtr &d1, const unity::scopes::Department::SCPtr &d2) -> bool {
            return loc(d1->label(), d2->label()) > 0;
    });
    dep->set_subdepartments(departments);

    return dep;
}

//
// creates department menu with narrowed-down list of subdepartments of current department, as
// returned by server call
unity::scopes::Department::SPtr click::Query::populate_departments(const click::DepartmentList& subdepts, const std::string& current_dep_id)
{
    // Return complete departments tree (starting from 'All') every time, rather than only parent - current - children; this is the only
    // way we can display siblings corrctly when navigating from Apps scope straight into a subdepartment of Store - see LP #1343242.
    // For currently visible department include its subdepartments as returned for current search by the server (subdepts) -
    // all others are constructed from the department lookup.
    return fromClickDepartment(impl->department_lookup.get_department_info(""), current_dep_id, subdepts);
}

// recursively store all departments in the departments database
void click::Query::store_departments(const click::DepartmentList& depts)
{
    assert(impl->depts_db);

    try
    {
        impl->depts_db->store_departments(depts, search_metadata().locale());
    }
    catch (const std::exception &e)
    {
        qWarning() << "Failed to update database: " << QString::fromStdString(e.what());
    }
}

void click::Query::push_package(const scopes::SearchReplyProxy& searchReply, scopes::Category::SCPtr category, const PackageSet &installedPackages, const Package& pkg)
{
    qDebug() << "pushing result" << QString::fromStdString(pkg.name);
    try {
        scopes::CategorisedResult res(category);
        res.set_title(pkg.title);
        res.set_art(pkg.icon_url);
        res.set_uri(pkg.url);
        res[click::Query::ResultKeys::NAME] = pkg.name;
        res["subtitle"] = pkg.publisher;
        auto installed = installedPackages.find(pkg);

        std::string price = _("FREE");
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1);
        ss << "☆ " << pkg.rating;
        std::string rating{ss.str()};

        bool was_purchased = false;
        time_t refundable_until = 0;
        double cur_price{0.00};
        auto suggested = impl->index.get_suggested_currency();
        std::string currency = Configuration::get_currency(suggested);
        if (pkg.prices.count(currency) == 1) {
            cur_price = pkg.prices.at(currency);
        } else {
            // NOTE: This is decprecated. Here for compatibility.
            currency = Configuration::CURRENCY_DEFAULT;
            cur_price = pkg.price;
        }
        res["price"] = scopes::Variant(cur_price);
        res[click::Query::ResultKeys::VERSION] = pkg.version;


        qDebug() << "App:" << pkg.name.c_str() << ", price:" << cur_price;
        std::string formatted_price;
        if (cur_price > 0.00f) {
            if (!Configuration::get_purchases_enabled()) {
                // Don't show priced apps if flag not set
                return;
            }
            // Check if the priced app was already purchased.
            auto purchased = purchased_apps.find({pkg.name});
            was_purchased = purchased != purchased_apps.end();
            if (was_purchased) {
                refundable_until = purchased->refundable_until;
            }
            qDebug() << "was purchased?" << was_purchased << ", refundable_until:" << refundable_until;

            // Get the currency symbol to use.
            QLocale locale;
            auto symbol = Configuration::CURRENCY_MAP.at(currency);
            formatted_price = locale.toCurrencyString(cur_price,
                                                      symbol.c_str()).toUtf8().data();
            res["currency_symbol"] = symbol;
        }
        if (installed != installedPackages.end()) {
            res[click::Query::ResultKeys::INSTALLED] = true;
            res[click::Query::ResultKeys::PURCHASED] = was_purchased;
            price = _("✔ INSTALLED");
            res[click::Query::ResultKeys::VERSION] = installed->version;
        } else if (was_purchased) {
            res[click::Query::ResultKeys::PURCHASED] = true;
            res[click::Query::ResultKeys::INSTALLED] = false;
            price = _("✔ PURCHASED");
        } else {
            res[click::Query::ResultKeys::INSTALLED] = false;
            res[click::Query::ResultKeys::PURCHASED] = false;
            if (cur_price > 0.00f) {
                price = formatted_price;
            }
        }

        res[click::Query::ResultKeys::REFUNDABLE_UNTIL] = unity::scopes::Variant((int64_t)refundable_until);
        res["formatted_price"] = formatted_price;
        res["price_area"] = price;
        res["rating"] = rating;

        // Add the price and rating as attributes.
        scopes::VariantBuilder builder;
        builder.add_tuple({
                {"value", scopes::Variant(price)},
            });
        builder.add_tuple({
                {"value", scopes::Variant("")},
            });
        builder.add_tuple({
                {"value", scopes::Variant(rating)},
            });
        builder.add_tuple({
                {"value", scopes::Variant("")},
            });
        res["attributes"] = builder.end();

        this->push_result(searchReply, res);
    } catch(const std::exception& e){
        qCritical() << "push_package: Exception: " << e.what() ;
    } catch(...){
        qDebug() << "no reason to catch";
    }
}

void click::Query::push_highlights(const scopes::SearchReplyProxy& searchReply, const HighlightList& highlights, const PackageSet &locallyInstalledApps)
{
    const scopes::CategoryRenderer renderer(CATEGORY_APPS_DISPLAY);
    const scopes::CategoryRenderer scopes_renderer(CATEGORY_SCOPES_DISPLAY);
    const scopes::CategoryRenderer aotw_renderer(CATEGORY_APP_OF_THE_WEEK);

    for (auto const& hl: highlights)
    {
        scopes::CategoryRenderer const* rdr = &renderer;
        if (hl.slug() == "app-of-the-week" || hl.packages().size() == 1)
        {
            rdr = &aotw_renderer;
        }
        if (hl.contains_scopes())
        {
            rdr = &scopes_renderer;
        }
        auto category = register_category(searchReply, hl.slug(), hl.name(), "", *rdr);
        for (auto const& pkg: hl.packages())
        {
            push_package(searchReply, category, locallyInstalledApps, pkg);
        }
    }
    qDebug() << "Highlights pushed";
}

void click::Query::push_departments(const scopes::SearchReplyProxy& searchReply, const scopes::Department::SCPtr& root)
{
    if (root != nullptr)
    {
        try
        {
            qDebug() << "pushing departments";
            searchReply->register_departments(root);
        }
        catch (const std::exception& e)
        {
            qWarning() << "Failed to register departments for query " << QString::fromStdString(query().query_string()) <<
                ", current department " << QString::fromStdString(query().department_id()) << ": " << e.what();
        }
    }
    else
    {
        qWarning() << "No departments data for query " << QString::fromStdString(query().query_string()) <<
            "', current department " << QString::fromStdString(query().department_id());
    }
}

void click::Query::push_departments(scopes::SearchReplyProxy const& searchReply)
{
    auto rootdep = impl->department_lookup.get_department_info("");
    if (!rootdep)
    {
        qWarning() << "No department information available";
        return;
    }

    auto subdepts = rootdep->sub_departments();
    auto root = populate_departments(subdepts, "");
    push_departments(searchReply, root);
}

//
// push highlights and departments
// use cached highlights for root department, otherwise run an async job for highlights of current department.
void click::Query::add_highlights(scopes::SearchReplyProxy const& searchReply, const PackageSet& locallyInstalledApps)
{
    auto curdep = impl->department_lookup.get_department_info(query().department_id());
    if (!curdep)
    {
        qWarning() << "No department information for current department" << QString::fromStdString(query().department_id());
        return;
    }

    if (query().department_id() == "") // top-level departments
    {
        auto subdepts = curdep->sub_departments();
        auto root = populate_departments(subdepts, query().department_id());
        push_departments(searchReply, root);

        qDebug() << "pushing cached highlights";
        push_highlights(searchReply, impl->highlights, locallyInstalledApps);
        this->finished(searchReply); //FIXME: this shouldn't be needed
    }
    else
    {
        qDebug() << "starting departments call for department" << QString::fromStdString(curdep->id()) << ", href" << QString::fromStdString(curdep->href());
        impl->search_operation = impl->index.departments(curdep->href(), [this, locallyInstalledApps, searchReply](const DepartmentList& depts,
                    const HighlightList& highlights, Index::Error error, int)
                {
                    if (error == click::Index::Error::NoError)
                    {
                        qDebug() << "departments call completed";
                        auto root = populate_departments(depts, query().department_id());
                        push_departments(searchReply, root);
                        push_highlights(searchReply, highlights, locallyInstalledApps);
                    }
                    else
                    {
                        qWarning() << "departments call failed";
                    }
                    this->finished(searchReply); //FIXME: this shouldn't be needed
                });
        }
}

void click::Query::add_available_apps(scopes::SearchReplyProxy const& searchReply,
                                      const PackageSet& installedPackages,
                                      const std::string& categoryTemplate)
{
    // this assertion is here to ensure unit tests are properly implemented.
    // this pointer is never null during normal execution.
    assert(searchReply);

    run_under_qt([=]()
    {
        auto search_cb = [this, searchReply, categoryTemplate, installedPackages](Packages packages, Packages recommends) {
                qDebug("search callback");

                const scopes::CategoryRenderer categoryRenderer(categoryTemplate);

                std::string cat_title(_("Available"));
                {
                    char tmp[512];
                    unsigned int num_results = static_cast<unsigned int>(packages.size());
                    if (snprintf(tmp, sizeof(tmp),
                                 dngettext(GETTEXT_PACKAGE,
                                           "%u result in Ubuntu Store",
                                           "%u results in Ubuntu Store",
                                           num_results), num_results) > 0) {
                        cat_title = tmp;
                    }
                }
                auto category = register_category(searchReply, "appstore", cat_title, "", categoryRenderer);

                const scopes::CategoryRenderer recommendsCatRenderer(categoryTemplate);
                auto recommendsCategory = register_category(searchReply, "recommends",
                                                _("Recommended"), "",
                                                recommendsCatRenderer);

                // handle packages data
                foreach (auto p, packages) {
                    push_package(searchReply, category, installedPackages, p);
                }
                foreach (auto r, recommends) {
                    push_package(searchReply, recommendsCategory,
                                 installedPackages, r);
                }
                qDebug() << "search completed";
                this->finished(searchReply); //FIXME: this shouldn't be needed
            };

            // this is the case when we do bootstrap for the first time, or it failed last time
            if (impl->department_lookup.size() == 0)
            {
                qDebug() << "performing bootstrap request";
                impl->search_operation = impl->index.bootstrap([this, search_cb, searchReply, installedPackages](const DepartmentList& deps, const
                            HighlightList& highlights, click::Index::Error error, int) {
                if (error == click::Index::Error::NoError)
                {
                    qDebug() << "bootstrap request completed";
                    auto root = std::make_shared<click::Department>("", _("All"), "", true);
                    root->set_subdepartments(deps);
                    DepartmentList rdeps { root };
                    impl->department_lookup.rebuild(rdeps);
                    impl->highlights = highlights;
                    qDebug() << "Total number of departments:" << impl->department_lookup.size() << ", highlights:" << highlights.size();

                    if (impl->depts_db)
                    {
                        qDebug() << "Storing departments in the database";
                        store_departments(deps);
                    }
                    else
                    {
                        qWarning() << "Departments db not available";
                    }
                }
                else
                {
                    qWarning() << "bootstrap request failed";
                }

                if (error == click::Index::Error::NoError)
                {
                    if (query().query_string().empty())
                    {
                        add_highlights(searchReply, installedPackages);
                    }
                    else
                    {
                        qDebug() << "starting search of" << QString::fromStdString(query().query_string());
                        const bool force_cache = (search_metadata().internet_connectivity() == scopes::QueryMetadata::ConnectivityStatus::Disconnected);
                        push_departments(searchReply);
                        impl->search_operation = impl->index.search(query().query_string(), query().department_id(), search_cb, force_cache);
                    }
                }
            });
        }
        else
        {
            if (query().query_string().empty())
            {
                add_highlights(searchReply, installedPackages);
            }
            else // normal search
            {
                qDebug() << "starting search of" << QString::fromStdString(query().query_string());
                push_departments(searchReply);
                impl->search_operation = impl->index.search(query().query_string(), query().department_id(), search_cb);
            }
        }
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
    auto q = query().query_string();
    std::string categoryTemplate = CATEGORY_APPS_SEARCH;
    if (q.empty()) {
        categoryTemplate = CATEGORY_APPS_DISPLAY;
    }
    if (Configuration::get_purchases_enabled()) {
        std::promise<pay::PurchaseSet> purchased_promise;
        std::future<pay::PurchaseSet> purchased_future = purchased_promise.get_future();
        qDebug() << "Getting list of purchased apps.";
        run_under_qt([this, &purchased_promise]() {
                impl->purchases_operation = impl->pay_package.get_purchases([&purchased_promise](const pay::PurchaseSet& purchases) {
                        purchased_promise.set_value(purchases);
                    });
            });
        purchased_apps = purchased_future.get();
    }

    add_available_apps(searchReply, get_installed_packages(), categoryTemplate);
}
