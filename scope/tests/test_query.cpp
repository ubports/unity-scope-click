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

#include <string>
#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "click/query.h"
#include "click/index.h"
#include "click/application.h"

#include "mock_network_access_manager.h"

#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CannedQuery.h>
#include <unity/scopes/SearchReply.h>

using namespace ::testing;

namespace
{
static const std::string FAKE_QUERY {"FAKE_QUERY"};
static const std::string FAKE_CATEGORY_TEMPLATE {"{}"};


class MockIndex : public click::Index {
    click::PackageList packages;
public:
    MockIndex(click::PackageList packages = click::PackageList())
        : Index(QSharedPointer<click::web::Client>()),
          packages(packages)
    {

    }

    click::Cancellable search(const std::string &query, std::function<void (click::PackageList)> callback) override
    {
        do_search(query, callback);
        callback(packages);
        return click::Cancellable();
    }

    MOCK_METHOD2(do_search,
                 void(const std::string&,
                      std::function<void(click::PackageList)>));
};

class MockQuery : public click::Query {
public:
    MockQuery(const std::string query, click::Index& index) : Query(query, index)
    {

    }
    void wrap_add_available_apps(const scopes::SearchReplyProxy &searchReply,
                                 const std::set<std::string> &locallyInstalledApps,
                                 const std::string& categoryTemplate)
    {
        add_available_apps(searchReply, locallyInstalledApps, categoryTemplate);
    }
    MOCK_METHOD2(push_result, bool(scopes::SearchReplyProxy const&, scopes::CategorisedResult const&));
    MOCK_METHOD5(register_category, scopes::Category::SCPtr(const scopes::SearchReplyProxy &searchReply,
                                                            const std::string &id,
                                                            const std::string &title,
                                                            const std::string &icon,
                                                            const scopes::CategoryRenderer &renderer_template));
};

class MockQueryRun : public click::Query {
public:
    MockQueryRun(const std::string query, click::Index& index) : Query(query, index)
    {

    }
    MOCK_METHOD3(add_available_apps,
                 void(scopes::SearchReplyProxy const&searchReply,
                      const std::set<std::string> &locallyInstalledApps,
                      const std::string& categoryTemplate));
    MOCK_METHOD3(push_local_results, void(scopes::SearchReplyProxy const &replyProxy,
                                          std::vector<click::Application> const &apps,
                                          std::string& categoryTemplate));
};

class FakeCategory : public scopes::Category
{
public:
    FakeCategory(std::string const& id, std::string const& title,
                 std::string const& icon, scopes::CategoryRenderer const& renderer) :
       scopes::Category(id, title, icon, renderer)
    {
    }

};
} // namespace

class QueryTest : public Test
{

};

// TODO: fails while creating a Category, should be fixed after updating unity-scopes-api
TEST_F(QueryTest, testAddAvailableAppsCallsClickIndex)
{
    MockIndex mock_index;
    std::set<std::string> no_installed_packages;
    MockQuery q(FAKE_QUERY, mock_index);
    EXPECT_CALL(mock_index, do_search(FAKE_QUERY, _)).Times(1);
    scopes::SearchReplyProxy reply;

    scopes::CategoryRenderer renderer("{}");
    FakeCategory cat("id", "", "", renderer);
    std::shared_ptr<FakeCategory> ptrCat(&cat);
    EXPECT_CALL(q, register_category(_, _, _, _, _)).WillOnce(Return(ptrCat));

    q.wrap_add_available_apps(reply, no_installed_packages, FAKE_CATEGORY_TEMPLATE);
}

TEST_F(QueryTest, testAddAvailableAppsPushesResults)
{
    click::PackageList packages {
        {"name", "title", "", "", ""}
    };
    MockIndex mock_index(packages);
    std::set<std::string> no_installed_packages;
    MockQuery q(FAKE_QUERY, mock_index);
    EXPECT_CALL(mock_index, do_search(FAKE_QUERY, _));

    scopes::CategoryRenderer renderer("{}");
    FakeCategory cat("id", "", "", renderer);
    std::shared_ptr<FakeCategory> ptrCat(&cat);
    EXPECT_CALL(q, register_category(_, _, _, _, _)).WillOnce(Return(ptrCat));

    scopes::SearchReplyProxy reply;
    auto expected_title = packages.front().title;
    EXPECT_CALL(q, push_result(_, Property(&scopes::CategorisedResult::title, expected_title)));
    q.wrap_add_available_apps(reply, no_installed_packages, FAKE_CATEGORY_TEMPLATE);
}

TEST_F(QueryTest, testAddAvailableAppsWithNullCategory)
{
    click::PackageList packages {
        {"name", "title", "", "", ""}
    };
    MockIndex mock_index(packages);
    std::set<std::string> no_installed_packages;
    MockQuery q(FAKE_QUERY, mock_index);
    EXPECT_CALL(mock_index, do_search(FAKE_QUERY, _));

    EXPECT_CALL(q, register_category(_, _, _, _, _)).WillOnce(Return(nullptr));

    scopes::SearchReplyProxy reply;
    EXPECT_CALL(q, push_result(_, _)).Times(0);
    q.wrap_add_available_apps(reply, no_installed_packages, FAKE_CATEGORY_TEMPLATE);
}

TEST_F(QueryTest, testQueryRunCallsAddAvailableApps)
{
    click::PackageList packages {
        {"name", "title", "", "", ""}
    };
    MockIndex mock_index(packages);
    std::set<std::string> no_installed_packages;
    MockQueryRun q(FAKE_QUERY, mock_index);
    auto reply = scopes::SearchReplyProxy();
    EXPECT_CALL(q, push_local_results(_, _, _));
    EXPECT_CALL(q, add_available_apps(reply, no_installed_packages, _));

    q.run(reply);
}

MATCHER_P(HasPackageName, n, "") { return arg[click::Query::ResultKeys::NAME].get_string() == n; }

TEST_F(QueryTest, testDuplicatesFilteredOnPackageName)
{
    click::PackageList packages {
        {"org.example.app1", "app title1", "", "", ""},
        {"org.example.app2", "app title2", "", "", ""}
    };
    MockIndex mock_index(packages);
    std::set<std::string> one_installed_package {
        "org.example.app2"
    };
    MockQuery q(FAKE_QUERY, mock_index);
    EXPECT_CALL(mock_index, do_search(FAKE_QUERY, _));

    scopes::CategoryRenderer renderer("{}");
    FakeCategory cat("id", "", "", renderer);
    std::shared_ptr<FakeCategory> ptrCat(&cat);
    EXPECT_CALL(q, register_category(_, _, _, _, _)).WillOnce(Return(ptrCat));

    scopes::SearchReplyProxy reply;
    auto expected_name = packages.front().name;
    EXPECT_CALL(q, push_result(_, HasPackageName(expected_name)));
    q.wrap_add_available_apps(reply, one_installed_package, FAKE_CATEGORY_TEMPLATE);
}
