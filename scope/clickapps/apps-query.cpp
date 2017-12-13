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

#include <click/application.h>
#include <click/departments-db.h>

#include <click/key_file_locator.h>

#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CannedQuery.h>
#include <unity/scopes/SearchReply.h>
#include <unity/scopes/SearchMetadata.h>
#include <unity/scopes/Department.h>

#include <vector>
#include <locale>

#include <click/click-i18n.h>
#include "apps-query.h"
#include <QDebug>

namespace
{

static const std::string CATEGORY_APPS_DISPLAY = R"(
    {
        "schema-version" : 1,
        "template" : {
            "category-layout" : "grid",
            "collapsed-rows": 0,
            "card-size": "small"
        },
        "components" : {
            "title" : "title",
            "art" : {
                "field": "art",
                "aspect-ratio": 1.6,
                "fill-mode": "fit",
                "fallback": "image://theme/placeholder-app-icon"
            }
        }
    }
)";

static const char CATEGORY_STORE[] = R"(
{
  "template": {
    "category-layout": "grid",
    "overlay": true,
    "card-size": "small",
    "card-background": "color:///#E95420"
  },
  "components": {
    "title": "title",
    "art": {
      "aspect-ratio": 0.55,
      "field": "art"
    },
    "overlay-color": "overlay-color"
  }
}

)";


}

click::apps::ResultPusher::ResultPusher(const scopes::SearchReplyProxy &replyProxy, const std::vector<std::string>& apps)
    :  replyProxy(replyProxy)
{
    for (auto const& app: apps)
    {
        //
        // click entries in the dconf key are expected to be in the format of
        // "foo.bar.package_appname"; split on underscore and just use the first part
        // for now (second part should be honored when we support multiple apps per package).
        auto i = app.find("_");
        if (i != std::string::npos)
        {
            const std::string pkg = app.substr(0, i);
            core_apps.push_back(pkg);
            top_apps_lookup.insert(pkg);
        }
        else
        {
            core_apps.push_back(app);
            top_apps_lookup.insert(app);
        }
    }
}

void click::apps::ResultPusher::push_result(scopes::Category::SCPtr& cat, const click::Application& a, bool lonely_result)
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
    res["lonely_result"] = lonely_result;
    replyProxy->push(res);
}

//
// Return an application identifier used to match applications against core-apps dconf key;
// For click apps, it just returns application name (e.g. com.canonical.calculator).
// For non-click apps, it return the desktop file name (without extension), taken from app uri.
std::string click::apps::ResultPusher::get_app_identifier(const click::Application& app)
{
    static const std::string app_prefix("application:///");
    if (!app.name.empty())
    {
        return app.name;
    }
    if (app.url.size() > app_prefix.size())
    {
        auto i = app.url.rfind('.');
        if (i != std::string::npos)
        {
            return app.url.substr(app_prefix.size(), i - app_prefix.size());
        }
    }
    throw std::runtime_error("Cannot determine application identifier for" + app.url);
}

void click::apps::ResultPusher::push_local_results(
                                      const std::vector<click::Application> &apps,
                                      const std::string &categoryTemplate,
                                      bool show_title)
{
    const scopes::CategoryRenderer rdr(categoryTemplate);
    auto cat = replyProxy->register_category("local", show_title ? _("Apps") : "", "", rdr);

    for(const auto & a: apps)
    {
        try
        {
            if (top_apps_lookup.size() == 0 || top_apps_lookup.find(get_app_identifier(a)) == top_apps_lookup.end())
            {
                push_result(cat, a, apps.size() == 1);
            }
        }
        catch (const std::runtime_error &e)
        {
            qWarning() << QString::fromStdString(e.what());
        }
    }
}

void click::apps::ResultPusher::push_top_results(
        const std::vector<click::Application>& apps,
        const std::string& categoryTemplate)
{
    const scopes::CategoryRenderer rdr(categoryTemplate);
    auto cat = replyProxy->register_category("predefined", "", "", rdr);

    //
    // iterate over all apps, insert those matching core apps into top_apps_to_push
    std::map<std::string, click::Application> top_apps_to_push;
    for (const auto& a: apps)
    {
        try
        {
            const auto id = get_app_identifier(a);
            if (top_apps_lookup.find(id) != top_apps_lookup.end())
            {
                top_apps_to_push[id] = a;
                if (core_apps.size() == top_apps_to_push.size())
                {
                    // no need to iterate over remaining apps
                    break;
                }
            }
        }
        catch (const std::runtime_error &e)
        {
            qWarning() << QString::fromStdString(e.what());
        }
    }

    //
    // iterate over core apps and insert them based on top_apps_to_push;
    // this way the order of core apps is preserved.
    for (const auto &a: core_apps)
    {
        auto const it = top_apps_to_push.find(a);
        if (it != top_apps_to_push.end())
        {
            push_result(cat, it->second);
        }
    }
}

struct click::apps::Query::Private
{
    Private(std::shared_ptr<click::DepartmentsDb> depts_db,
            const scopes::SearchMetadata& metadata,
            std::shared_future<void> const& qt_ready)
        : depts_db(depts_db),
          meta(metadata),
          qt_ready_(qt_ready)
    {
    }

    std::shared_ptr<click::DepartmentsDb> depts_db;
    scopes::SearchMetadata meta;
    click::Configuration configuration;
    std::shared_future<void> qt_ready_;
};

click::apps::Query::Query(unity::scopes::CannedQuery const& query,
                          std::shared_ptr<DepartmentsDb> depts_db,
                          scopes::SearchMetadata const& metadata,
                          std::shared_future<void> const& qt_ready)
    : unity::scopes::SearchQueryBase(query, metadata),
      impl(new Private(depts_db, metadata, qt_ready))
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

click::Interface& click::apps::Query::clickInterfaceInstance()
{
    static QSharedPointer<click::KeyFileLocator> keyFileLocator(new click::KeyFileLocator());
    static click::Interface iface(keyFileLocator);

    return iface;
}

void click::apps::Query::push_local_departments(scopes::SearchReplyProxy const& replyProxy, const std::vector<Application>& apps)
{
    auto const current_dep_id = query().department_id();
    const std::list<std::string> locales = { search_metadata().locale(), "en_US" };

    //
    // create helper lookup of all departments of currently installed apps.
    // note that apps that are passed here are supposed to be already filterd by current department
    // that means we only have subdepartments of current department (or subdepartment(s) of subdepartment(s)
    // and so on of current department, as the hierarchy may be of arbitrary depth.
    std::unordered_set<std::string> all_subdepartments;
    for (auto const app: apps)
    {
        all_subdepartments.insert(app.real_department);
    }

    unity::scopes::Department::SPtr root;

    try
    {
        static const std::string all_dept_name = _("All");

        // create node for current department
        auto name = current_dep_id == "" ? all_dept_name : impl->depts_db->get_department_name(current_dep_id, locales);
        unity::scopes::Department::SPtr current = unity::scopes::Department::create(current_dep_id, query(), name);

        unity::scopes::DepartmentList children;

        // attach subdepartments to it
        for (auto const& subdep: impl->depts_db->get_children_departments(current_dep_id))
        {
            //
            // check if this subdepartment either directly matches a subdepartment of installed app,
            // or is any of the departments of installed apps is a descendant of current subdepartment.
            // the latter means we have app somewhere in the subtree of the current subdepartment, so it
            // needs to be shown.
            bool show_subdepartment = false;
            auto it = all_subdepartments.find(subdep.id);
            if (it != all_subdepartments.end())
            {
                // subdepartment id matches directly one of the ids from all_subdepartments
                show_subdepartment = true;
                all_subdepartments.erase(it);
            }
            else
            {
                // no direct match - we need to check descendants of this subdepartment
                // by querying the db
                for (auto it = all_subdepartments.begin(); it != all_subdepartments.end(); it++)
                {
                    if (impl->depts_db->is_descendant_of_department(*it, subdep.id))
                    {
                        show_subdepartment = true;
                        all_subdepartments.erase(it);
                        break;
                    }
                }
            }

            if (show_subdepartment)
            {
                // if single supdepartment fails, then ignore it and continue with others
                try
                {
                    name = impl->depts_db->get_department_name(subdep.id, locales);
                    unity::scopes::Department::SPtr dep = unity::scopes::Department::create(subdep.id, query(), name);
                    dep->set_has_subdepartments(subdep.has_children);
                    children.push_back(dep);
                }
                catch (const std::exception &e)
                {
                    qWarning() << "Failed to create subdeparment:" << QString::fromStdString(e.what());
                }
            }
        }

        if (children.size() > 0)
        {
            const std::locale loc("");
            children.sort([&loc](const unity::scopes::Department::SCPtr &d1, const unity::scopes::Department::SCPtr &d2) -> bool {
                    return loc(d1->label(), d2->label()) > 0;
                });
            current->set_subdepartments(children);
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

void click::apps::Query::run(scopes::SearchReplyProxy const& searchReply)
{
    if (impl->qt_ready_.valid())
        impl->qt_ready_.wait();

    const std::string categoryTemplate = CATEGORY_APPS_DISPLAY;
    auto const current_dept = query().department_id();
    auto const querystr = query().query_string();

    const bool show_top_apps = querystr.empty() && current_dept.empty();
    ResultPusher pusher(searchReply, show_top_apps ? impl->configuration.get_core_apps() : std::vector<std::string>());
    auto const ignoredApps = impl->configuration.get_ignored_apps();
    auto const localResults = clickInterfaceInstance().find_installed_apps(querystr, ignoredApps, current_dept, impl->depts_db);

    if (impl->depts_db)
    {
        push_local_departments(searchReply, localResults);
    }

    if (show_top_apps)
    {
        pusher.push_top_results(localResults, categoryTemplate);
    }

    const bool show_cat_title = current_dept.empty() && querystr.empty();
    pusher.push_local_results(
        localResults,
        categoryTemplate,
        show_cat_title);
}
