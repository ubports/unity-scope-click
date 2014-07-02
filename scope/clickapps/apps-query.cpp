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
#include <click/departments-db.h>

#include <click/key_file_locator.h>

#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CannedQuery.h>
#include <unity/scopes/SearchReply.h>
#include <unity/scopes/SearchMetadata.h>
#include <unity/scopes/Department.h>

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

void click::Query::push_local_results(scopes::SearchReplyProxy const &replyProxy,
                                      std::vector<click::Application> const &apps,
                                      std::string &categoryTemplate)
{
    scopes::CategoryRenderer rdr(categoryTemplate);
    auto cat = replyProxy->register_category("local", _("My apps"), "", rdr);

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
    Private(click::Index& index, std::shared_ptr<click::DepartmentsDb> depts_db, const scopes::SearchMetadata& metadata)
        : index(index),
          depts_db(depts_db),
          meta(metadata)
    {
    }
    click::Index& index;
    std::shared_ptr<click::DepartmentsDb> depts_db;
    scopes::SearchMetadata meta;
};

click::Query::Query(unity::scopes::CannedQuery const& query, click::Index& index, std::shared_ptr<DepartmentsDb> depts_db,
        scopes::SearchMetadata const& metadata)
    : unity::scopes::SearchQueryBase(query, metadata),
      impl(new Private(index, depts_db, metadata))
{
}

void click::Query::cancelled()
{
    qDebug() << "cancelling search of" << QString::fromStdString(query().query_string());
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

void click::Query::add_fake_store_app(scopes::SearchReplyProxy const& searchReply)
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
        res[click::Query::ResultKeys::NAME] = title;
        res[click::Query::ResultKeys::DESCRIPTION] = "";
        res[click::Query::ResultKeys::MAIN_SCREENSHOT] = "";
        res[click::Query::ResultKeys::INSTALLED] = true;
        res[click::Query::ResultKeys::VERSION] = "";
        searchReply->push(res);
    }
}

void click::Query::push_local_departments(scopes::SearchReplyProxy const& replyProxy)
{
    auto const current_dep_id = query().department_id();
    const std::list<std::string> locales = { search_metadata().locale(), "en_US", "" };

    unity::scopes::Department::SPtr root;

    try
    {
        static const std::string all_dept_name = _("All departments");

        // create node for current department
        auto name = current_dep_id == "" ? all_dept_name : impl->depts_db->get_department_name(current_dep_id, locales);
        unity::scopes::Department::SPtr current = unity::scopes::Department::create(current_dep_id, query(), name);

        // attach subdepartments to it
        for (auto const& subdep: impl->depts_db->get_children_departments(current_dep_id))
        {
            name = impl->depts_db->get_department_name(subdep.id, locales);
            unity::scopes::Department::SPtr dep = unity::scopes::Department::create(subdep.id, query(), name);
            dep->set_has_subdepartments(subdep.has_children);
            current->add_subdepartment(dep);
        }

        // if current is not the top, then gets its parent
        if (current_dep_id != "")
        {
            auto const parent_dep_id = impl->depts_db->get_parent_department_id(current_dep_id);
            root = unity::scopes::Department::create(parent_dep_id, query(), parent_dep_id == "" ? all_dept_name :
                    impl->depts_db->get_department_name(parent_dep_id, locales));
            root->add_subdepartment(current);
        }
        else
        {
            root = current;
        }

        replyProxy->register_departments(root);
    }
    catch (const std::exception& e)
    {
        qWarning() << "Failed to push departments: " << QString::fromStdString(e.what());
    }
}

void click::Query::run(scopes::SearchReplyProxy const& searchReply)
{
    auto const querystr = query().query_string();
    auto const current_dept = query().department_id();

    std::string categoryTemplate = CATEGORY_APPS_SEARCH;
    if (querystr.empty()) {
        categoryTemplate = CATEGORY_APPS_DISPLAY;
        if (impl->depts_db)
        {
            push_local_departments(searchReply);
        }
    }

    //
    // get the set of packages that belong to current deparment;
    // only apply department filtering if not in root of all departments.
    bool apply_department_filter = (querystr.empty() && !current_dept.empty());
    std::unordered_set<std::string> pkgs_in_department;
    if (impl->depts_db && apply_department_filter)
    {
        try
        {
            pkgs_in_department = impl->depts_db->get_packages_for_department(current_dept);
        }
        catch (const std::exception& e)
        {
            qWarning() << "Failed to get packages of department" << QString::fromStdString(current_dept);
            apply_department_filter = false; // disable so that we are not loosing any apps if something goes wrong
        }
    }

    auto const localResults = clickInterfaceInstance().find_installed_apps(
                querystr, pkgs_in_department, apply_department_filter);

    push_local_results(
        searchReply,
        localResults,
        categoryTemplate);

    add_fake_store_app(searchReply);
}
