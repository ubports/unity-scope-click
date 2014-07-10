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

#ifndef STORE_QUERY_H
#define STORE_QUERY_H

#include "pay.h"

#include <unity/scopes/SearchQueryBase.h>
#include <unity/scopes/Department.h>

namespace scopes = unity::scopes;

#include <QSharedPointer>
#include <set>

#include <click/department-lookup.h>
#include <click/package.h>
#include <click/highlights.h>
#include <click/interface.h>

namespace click
{

class Application;
class Index;
class DepartmentLookup;

class Query : public scopes::SearchQueryBase
{
public:
    struct ResultKeys
    {
        ResultKeys() = delete;

        constexpr static const char* NAME{"name"};
        constexpr static const char* DESCRIPTION{"description"};
        constexpr static const char* MAIN_SCREENSHOT{"main_screenshot"};
        constexpr static const char* INSTALLED{"installed"};
        constexpr static const char* PURCHASED{"purchased"};
        constexpr static const char* DOWNLOAD_URL{"download_url"};
        constexpr static const char* VERSION{"version"};
    };

    Query(unity::scopes::CannedQuery const& query, click::Index& index, click::DepartmentLookup& dept_lookup, click::HighlightList& highlights,
            scopes::SearchMetadata const& metadata);
    virtual ~Query();

    virtual void cancelled() override;

    virtual void run(scopes::SearchReplyProxy const& reply) override;

protected:
    virtual void populate_departments(const click::DepartmentList& depts, const std::string& current_department_id, unity::scopes::Department::SPtr &root);
    virtual void push_departments(const scopes::SearchReplyProxy& searchReply, const scopes::Department::SCPtr& root);
    virtual void add_highlights(scopes::SearchReplyProxy const& searchReply, const PackageSet& installedPackages);
    virtual void add_available_apps(const scopes::SearchReplyProxy &searchReply, const PackageSet &installedPackages, const std::string &category);
    virtual click::Interface& clickInterfaceInstance();
    virtual PackageSet get_installed_packages();
    virtual bool push_result(const scopes::SearchReplyProxy &searchReply, scopes::CategorisedResult const& res);
    virtual void finished(const scopes::SearchReplyProxy &searchReply);
    virtual scopes::Category::SCPtr register_category(scopes::SearchReplyProxy const& searchReply,
                                               std::string const& id,
                                               std::string const& title,
                                               std::string const& icon,
                                               scopes::CategoryRenderer const& renderer_template);
    virtual void push_package(const scopes::SearchReplyProxy& searchReply, scopes::Category::SCPtr category, const PackageSet &locallyInstalledApps,
            const click::Package& pkg);
    virtual void push_highlights(const scopes::SearchReplyProxy& searchReply, const HighlightList& highlights, const PackageSet &locallyInstalledApps);
    virtual void run_under_qt(const std::function<void()> &task);

    pay::Package pay_package;

private:
    struct Private;
    QSharedPointer<Private> impl;
};
}

#endif // CLICK_QUERY_H
