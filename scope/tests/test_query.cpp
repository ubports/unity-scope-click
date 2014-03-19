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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "click/query.h"
#include "click/index.h"

#include "mock_network_access_manager.h"

#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CannedQuery.h>
#include <unity/scopes/SearchReply.h>

using namespace ::testing;

namespace
{
static const std::string FAKE_QUERY {"FAKE_QUERY"};

class MockIndex : public click::Index {
    click::PackageList packages;
public:
    MockIndex(click::PackageList packages = click::PackageList())
        : Index(QSharedPointer<click::web::Client>()),
          packages(packages)
    {

    }

    click::Cancellable search(const std::string &query, std::function<void (click::PackageList)> callback)
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
                                 const std::shared_ptr<const unity::scopes::Category> category)
    {
        add_available_apps(searchReply, locallyInstalledApps, category);
    }
    MOCK_METHOD2(push_result, bool(scopes::SearchReplyProxy const&, scopes::CategorisedResult const&));
};

class QueryTest : public ::testing::Test
{

};

class FakeCategory : public scopes::Category
{
    scopes::CategoryRenderer renderer;
public:
    FakeCategory(std::string const& id, std::string const& title,
                 std::string const &icon) :
       scopes::Category(id, title, icon, renderer)
    {
    }

};
} // namespace

// TODO: fails while creating a Category, should be fixed after updating unity-scopes-api
TEST_F(QueryTest, DISABLED_testAddAvailableAppsCallsClickIndex)
{
    MockIndex mock_index;
    std::set<std::string> no_installed_packages;
    MockQuery q(FAKE_QUERY, mock_index);
    EXPECT_CALL(mock_index, do_search(FAKE_QUERY, _)).Times(1);
    scopes::SearchReplyProxy searchReply;
    auto category = searchReply->register_category("appstore", "Available", "");

    q.wrap_add_available_apps(searchReply, no_installed_packages, category);
}

TEST_F(QueryTest, testAddAvailableAppsCallbackReceivesPackages)
{
    click::PackageList packages {
        {"name", "title", "", "", ""}
    };
    MockIndex mock_index(packages);
    std::set<std::string> no_installed_packages;
    MockQuery q(FAKE_QUERY, mock_index);
    EXPECT_CALL(mock_index, do_search(FAKE_QUERY, _)).Times(1);

    auto reply = scopes::SearchReplyProxy();
    FakeCategory category("appstore", "Available", "");
    std::shared_ptr<scopes::Category> ptrCat(&category);
    EXPECT_CALL(q, push_result(_, _));
    q.wrap_add_available_apps(reply, no_installed_packages, ptrCat);
    auto name = packages.front().name;
//    auto front = reply->front();
//    auto found_name = front[click::Query::ResultKeys::NAME].get_string();
//    EXPECT_EQ(name, found_name);
}
