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

#include <clickapps/apps-query.h>
#include <click/departments-db.h>

#include <unity/scopes/SearchReply.h>
#include <unity/scopes/SearchMetadata.h>
#include <unity/scopes/CannedQuery.h>
#include <unity/scopes/testing/MockSearchReply.h>

#include "test_helpers.h"

using namespace click::test::helpers;
using namespace ::testing;

class ResultPusherTest : public ::testing::Test
{
protected:
    scopes::SearchReplyProxy reply;
public:
    ResultPusherTest()
    {
        reply.reset(new scopes::testing::MockSearchReply());
    }
};

class MockClickInterface : public click::Interface
{
public:
    MockClickInterface() = default;
    MOCK_METHOD3(find_installed_apps, std::vector<click::Application>(const std::string&, const std::string&, const std::shared_ptr<click::DepartmentsDb>&));
};

class MockAppsQuery : public click::apps::Query
{
private:
    std::shared_ptr<MockClickInterface> click_iface;

public:
    MockAppsQuery(unity::scopes::CannedQuery const& query, std::shared_ptr<click::DepartmentsDb> depts_db, scopes::SearchMetadata const& metadata,
            const std::shared_ptr<MockClickInterface>& click_iface)
        : click::apps::Query(query, depts_db, metadata),
          click_iface(click_iface)
    {
    }

    click::Interface& clickInterfaceInstance() override
    {
        return *click_iface;
    }
};

MATCHER_P(HasApplicationTitle, n, "") { return arg["title"].get_string() == n; }

TEST_F(ResultPusherTest, testPushTopAndLocalResults)
{
    std::string categoryTemplate("{}");
    std::vector<click::Application> apps {
        {"app1", "App1", 0.0f, "icon", "url", "", "sshot", ""},
        {"app2", "App2", 0.0f, "icon", "url", "", "sshot", ""},
        {"app3", "App3", 0.0f, "icon", "url", "", "sshot", ""},
        {"", "App4", 0.0f, "icon", "application:///app4.desktop", "", "sshot", ""} // a non-click app
    };

    click::apps::ResultPusher pusher(reply, {"app2_fooappname", "app4"});
    auto mockreply = (scopes::testing::MockSearchReply*)reply.get();

    scopes::CategoryRenderer renderer("{}");
    auto ptrCat = std::make_shared<FakeCategory>("id", "", "", renderer);

    EXPECT_CALL(*mockreply, register_category(_, _, _, _)).WillRepeatedly(Return(ptrCat));
    EXPECT_CALL(*mockreply, push(Matcher<unity::scopes::CategorisedResult const&>(HasApplicationTitle(std::string("App2")))));
    EXPECT_CALL(*mockreply, push(Matcher<unity::scopes::CategorisedResult const&>(HasApplicationTitle(std::string("App4")))));

    EXPECT_CALL(*mockreply, push(Matcher<unity::scopes::CategorisedResult const&>(HasApplicationTitle(std::string("App1")))));
    EXPECT_CALL(*mockreply, push(Matcher<unity::scopes::CategorisedResult const&>(HasApplicationTitle(std::string("App3")))));
    pusher.push_top_results(apps, categoryTemplate);
    pusher.push_local_results(apps, categoryTemplate, true);
}

MATCHER_P(ResultUriMatchesCannedQuery, q, "") {
    auto const query = unity::scopes::CannedQuery::from_uri(arg.uri());
    return query.scope_id() == q.scope_id()
        && query.query_string() == q.query_string()
        && query.department_id() == q.department_id();
}

MATCHER_P(CategoryTitleContains, s, "") { return arg.find(s) != std::string::npos; }

TEST(Query, testUbuntuStoreFakeResult)
{
    const scopes::SearchMetadata metadata("en_EN", "phone");
    const unity::scopes::CannedQuery query("foo.scope", "FooBar", "");
    const unity::scopes::CannedQuery query2("foo.scope", "Metallica", "");

    click::apps::Query q(query, nullptr, metadata);
    click::apps::Query q2(query2, nullptr, metadata);

    scopes::testing::MockSearchReply mock_reply;
    scopes::SearchReplyProxy reply(&mock_reply, [](unity::scopes::SearchReply*){});
    scopes::CategoryRenderer renderer("{}");
    auto ptrCat = std::make_shared<FakeCategory>("id", "", "", renderer);

    const unity::scopes::CannedQuery target_query("com.canonical.scopes.clickstore", "FooBar", "");

    EXPECT_CALL(mock_reply, register_category("store", CategoryTitleContains("FooBar"), _, _)).WillOnce(Return(ptrCat));
    EXPECT_CALL(mock_reply, push(Matcher<const unity::scopes::CategorisedResult&>(ResultUriMatchesCannedQuery(target_query))));

    scopes::testing::MockSearchReply mock_reply2;
    scopes::SearchReplyProxy reply2(&mock_reply2, [](unity::scopes::SearchReply*){});

    const unity::scopes::CannedQuery target_query2("com.canonical.scopes.clickstore", "Metallica", "");

    EXPECT_CALL(mock_reply2, register_category("store", CategoryTitleContains("Metallica"), _, _)).WillOnce(Return(ptrCat));
    EXPECT_CALL(mock_reply2, push(Matcher<const unity::scopes::CategorisedResult&>(ResultUriMatchesCannedQuery(target_query2))));

    q.add_fake_store_app(reply);
    q2.add_fake_store_app(reply2);
}

TEST(Query, testUbuntuStoreFakeResultWithDepartment)
{
    const scopes::SearchMetadata metadata("en_EN", "phone");
    const unity::scopes::CannedQuery query("foo.scope", "", "music-department");

    click::apps::Query q(query, nullptr, metadata);

    scopes::CategoryRenderer renderer("{}");
    auto ptrCat = std::make_shared<FakeCategory>("id", "", "", renderer);

    scopes::testing::MockSearchReply mock_reply;
    scopes::SearchReplyProxy reply(&mock_reply, [](unity::scopes::SearchReply*){});

    const unity::scopes::CannedQuery target_query("com.canonical.scopes.clickstore", "", "music-department");

    EXPECT_CALL(mock_reply, register_category("store", _, _, _)).WillOnce(Return(ptrCat));
    EXPECT_CALL(mock_reply, push(Matcher<const unity::scopes::CategorisedResult&>(ResultUriMatchesCannedQuery(target_query))));

    q.add_fake_store_app(reply);
}

// this matcher expects a list of department ids in depts:
// first on the list is the root, followed by children ids.
// the arg of the matcher is unity::scopes::Department ptr.
MATCHER_P(MatchesDepartments, depts, "") {
    auto it = depts.begin();
    if (arg->id() != *it)
        return false;
    auto const subdeps = arg->subdepartments();
    if (subdeps.size() != depts.size() - 1)
        return false;
    for (auto const& sub: subdeps)
    {
        if (sub->id() != *(++it))
            return false;
    }
    return true;
}

class DepartmentsTest : public ::testing::Test {
protected:
    const std::vector<click::Application> installed_apps = {
        {"app1", "App1", 0.0f, "icon", "url", "descr", "scrshot", "", "games"},
        {"app2", "App2", 0.0f, "icon", "url", "descr", "scrshot", "", "video"}
    };
    const scopes::SearchMetadata metadata{"en_EN", "phone"};
    const scopes::CategoryRenderer renderer{"{}"};
    const std::list<std::string> expected_locales {"en_EN", "en_US"};

};

TEST_F(DepartmentsTest, testRootDepartment)
{
    auto clickif = std::make_shared<MockClickInterface>();
    auto ptrCat = std::make_shared<FakeCategory>("id", "", "", renderer);
    auto depts_db = std::make_shared<MockDepartmentsDb>(":memory:", true);

    // query for root of the departments tree
    {
        const unity::scopes::CannedQuery query("foo.scope", "", "");

        MockAppsQuery q(query, depts_db, metadata, clickif);

        scopes::testing::MockSearchReply mock_reply;
        scopes::SearchReplyProxy reply(&mock_reply, [](unity::scopes::SearchReply*){});

        // no apps in 'books' department, thus excluded
        std::list<std::string> expected_departments({{"", "games", "video"}});

        EXPECT_CALL(*clickif, find_installed_apps(_, _, _)).WillOnce(Return(installed_apps));
        EXPECT_CALL(mock_reply, register_category("predefined", _, _, _)).WillOnce(Return(ptrCat));
        EXPECT_CALL(mock_reply, register_category("local", StrNe(""), _, _)).WillOnce(Return(ptrCat));
        EXPECT_CALL(mock_reply, register_category("store", _, _, _)).WillOnce(Return(ptrCat));
        EXPECT_CALL(mock_reply, register_departments(MatchesDepartments(expected_departments)));

        EXPECT_CALL(mock_reply, push(Matcher<unity::scopes::CategorisedResult const&>(_))).Times(3).WillRepeatedly(Return(true));

        EXPECT_CALL(*depts_db, get_department_name("games", expected_locales)).WillOnce(Return("Games"));
        EXPECT_CALL(*depts_db, get_department_name("video", expected_locales)).WillOnce(Return("Video"));
        EXPECT_CALL(*depts_db, is_empty("games")).WillRepeatedly(Return(false));
        EXPECT_CALL(*depts_db, is_empty("video")).WillRepeatedly(Return(false));
        EXPECT_CALL(*depts_db, is_descendant_of_department("games", "")).WillRepeatedly(Return(true));
        EXPECT_CALL(*depts_db, is_descendant_of_department("books", "")).WillRepeatedly(Return(true));
        EXPECT_CALL(*depts_db, is_descendant_of_department("video", "")).WillRepeatedly(Return(true));
        EXPECT_CALL(*depts_db, get_children_departments("")).WillOnce(Return(
                    std::list<click::DepartmentsDb::DepartmentInfo>({
                        {"games", false},
                        {"video", true},
                        {"books", true}
                    }))
                );

        q.run(reply);
    }
}

TEST_F(DepartmentsTest, testLeafDepartment)
{
    auto clickif = std::make_shared<MockClickInterface>();
    auto ptrCat = std::make_shared<FakeCategory>("id", "", "", renderer);
    auto depts_db = std::make_shared<MockDepartmentsDb>(":memory:", true);

    // query for a leaf department
    {
        const unity::scopes::CannedQuery query("foo.scope", "", "games");

        MockAppsQuery q(query, depts_db, metadata, clickif);

        scopes::testing::MockSearchReply mock_reply;
        scopes::SearchReplyProxy reply(&mock_reply, [](unity::scopes::SearchReply*){});

        std::list<std::string> expected_departments({"", "games"});

        EXPECT_CALL(*clickif, find_installed_apps(_, _, _)).WillOnce(Return(installed_apps));
        EXPECT_CALL(mock_reply, register_category("local", StrEq(""), _, _)).WillOnce(Return(ptrCat));
        EXPECT_CALL(mock_reply, register_category("store", _, _, _)).WillOnce(Return(ptrCat));
        EXPECT_CALL(mock_reply, register_departments(MatchesDepartments(expected_departments)));

        EXPECT_CALL(mock_reply, push(Matcher<unity::scopes::CategorisedResult const&>(_))).Times(3).WillRepeatedly(Return(true));

        EXPECT_CALL(*depts_db, get_parent_department_id("games")).WillOnce(Return(""));
        EXPECT_CALL(*depts_db, get_department_name("games", expected_locales)).WillOnce(Return("Games"));
        EXPECT_CALL(*depts_db, get_children_departments("games")).WillOnce(Return(
                    std::list<click::DepartmentsDb::DepartmentInfo>({})
                ));

        q.run(reply);
    }
}

TEST_F(DepartmentsTest, testNoDepartmentSearch)
{
    auto clickif = std::make_shared<MockClickInterface>();
    auto ptrCat = std::make_shared<FakeCategory>("id", "", "", renderer);
    auto depts_db = std::make_shared<MockDepartmentsDb>(":memory:", true);

    // query for department-less search
    {
        const unity::scopes::CannedQuery query("foo.scope", "App", "");

        MockAppsQuery q(query, depts_db, metadata, clickif);

        scopes::testing::MockSearchReply mock_reply;
        scopes::SearchReplyProxy reply(&mock_reply, [](unity::scopes::SearchReply*){});

        EXPECT_CALL(*clickif, find_installed_apps(_, _, _)).WillOnce(Return(installed_apps));
        EXPECT_CALL(mock_reply, register_category("local", StrEq(""), _, _)).WillOnce(Return(ptrCat));
        EXPECT_CALL(mock_reply, register_category("store", _, _, _)).WillOnce(Return(ptrCat));

        EXPECT_CALL(mock_reply, push(Matcher<unity::scopes::CategorisedResult const&>(_))).Times(3).WillRepeatedly(Return(true));

        q.run(reply);
    }
}

