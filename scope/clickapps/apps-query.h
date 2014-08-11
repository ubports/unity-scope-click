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

#ifndef APPS_QUERY_H
#define APPS_QUERY_H


#include <unity/scopes/SearchQueryBase.h>

namespace scopes = unity::scopes;

#include <QSharedPointer>
#include <set>
#include <unordered_set>
#include <click/interface.h>

namespace click
{

class Application;
class Configuration;
class DepartmentsDb;

namespace apps
{

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
        constexpr static const char* VERSION{"version"};
    };

    Query(unity::scopes::CannedQuery const& query, std::shared_ptr<DepartmentsDb> depts_db, scopes::SearchMetadata const& metadata);
    virtual ~Query();

    virtual void cancelled() override;

    virtual void run(scopes::SearchReplyProxy const& reply) override;

    virtual void add_fake_store_app(scopes::SearchReplyProxy const &replyProxy);

    virtual void push_local_departments(scopes::SearchReplyProxy const& replyProxy);

protected:
    virtual click::Interface& clickInterfaceInstance();

private:
    struct Private;
    QSharedPointer<Private> impl;
};

class ResultPusher
{
    const scopes::SearchReplyProxy &replyProxy;
    std::vector<std::string> core_apps;
    std::unordered_set<std::string> top_apps_lookup;

public:
    ResultPusher(const scopes::SearchReplyProxy &replyProxy, const std::vector<std::string>& core_apps);
    virtual ~ResultPusher() = default;

    virtual void push_local_results(const std::vector<click::Application> &apps,
                                    const std::string& categoryTemplate,
                                    bool show_title);

    virtual void push_top_results(
            const std::vector<click::Application>& apps,
            const std::string& categoryTemplate);
protected:
    virtual void push_result(scopes::Category::SCPtr& cat, const click::Application& a);
    static std::string get_app_identifier(const click::Application& app);
};
} // namespace apps
} // namespace query

#endif // CLICK_QUERY_H
