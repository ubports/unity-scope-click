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

#include "clickstore/pay.h"
#include "clickstore/store-query.h"

#include <string>
#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "click/qtbridge.h"
#include "click/index.h"
#include "click/application.h"

#include <tests/mock_network_access_manager.h>

#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CannedQuery.h>
#include <unity/scopes/ScopeBase.h>
#include <unity/scopes/SearchReply.h>
#include <unity/scopes/testing/MockSearchReply.h>

using namespace ::testing;
using namespace click;

namespace
{
static const std::string FAKE_QUERY {"FAKE_QUERY"};
static const std::string FAKE_CATEGORY_TEMPLATE {"{}"};


class MockPayPackage : public pay::Package {
public:
    MockPayPackage()
    {
    }

    bool verify(const std::string& pkg_name)
    {
        return do_verify(pkg_name);
    }

    MOCK_METHOD1(do_verify,
                 bool(const std::string&));
};

class MockIndex : public click::Index {
    click::Packages packages;
    click::Packages recommends;
    click::DepartmentList departments;
    click::DepartmentList bootstrap_departments;
    click::HighlightList bootstrap_highlights;

public:
    MockIndex(click::Packages packages = click::Packages(),
            click::DepartmentList departments = click::DepartmentList(),
            click::DepartmentList boot_departments = click::DepartmentList())
        : Index(QSharedPointer<click::web::Client>()),
          packages(packages),
          departments(departments),
          bootstrap_departments(boot_departments)
    {

    }

    click::web::Cancellable search(const std::string &query, std::function<void (click::Packages, click::Packages)> callback) override
    {
        do_search(query, callback);
        callback(packages, recommends);
        return click::web::Cancellable();
    }


    click::web::Cancellable bootstrap(std::function<void(const click::DepartmentList&, const click::HighlightList&, Error, int)> callback) override
    {
        callback(bootstrap_departments, bootstrap_highlights, click::Index::Error::NoError, 0);
        return click::web::Cancellable();
    }

    MOCK_METHOD2(do_search,
                 void(const std::string&,
                      std::function<void(click::Packages, click::Packages)>));
};

class MockQueryBase : public click::Query {
public:
    MockQueryBase(const unity::scopes::CannedQuery& query, click::Index& index,
                  click::DepartmentLookup& depts,
                  click::HighlightList& highlights,
                  scopes::SearchMetadata const& metadata) : click::Query(query, index, depts, highlights, metadata)
    {
        pay_package.reset(new MockPayPackage{});
    }

    void run_under_qt(const std::function<void()> &task) {
        // when testing, do not actually run under qt
        task();
    }
};

class MockQuery : public MockQueryBase {
public:
    MockQuery(const unity::scopes::CannedQuery& query, click::Index& index,
              click::DepartmentLookup& depts,
              click::HighlightList& highlights,
              scopes::SearchMetadata const& metadata) : MockQueryBase(query, index, depts, highlights, metadata)
    {
    }
    void wrap_add_available_apps(const scopes::SearchReplyProxy &searchReply,
                                 const PackageSet &installedPackages,
                                 const std::string& categoryTemplate)
    {
        add_available_apps(searchReply, installedPackages, categoryTemplate);
    }
    MOCK_METHOD2(push_result, bool(scopes::SearchReplyProxy const&, scopes::CategorisedResult const&));
    MOCK_METHOD0(clickInterfaceInstance, click::Interface&());
    MOCK_METHOD1(finished, void(scopes::SearchReplyProxy const&));
    MOCK_METHOD5(register_category, scopes::Category::SCPtr(const scopes::SearchReplyProxy &searchReply,
                                                            const std::string &id,
                                                            const std::string &title,
                                                            const std::string &icon,
                                                            const scopes::CategoryRenderer &renderer_template));
    using click::Query::get_installed_packages; // allow tests to access protected method
};

class MockQueryRun : public MockQueryBase {
public:
    MockQueryRun(const unity::scopes::CannedQuery& query, click::Index& index,
                 click::DepartmentLookup& depts,
                 click::HighlightList& highlights, 
                 scopes::SearchMetadata const& metadata) : MockQueryBase(query, index, depts, highlights, metadata)
    {

    }
    MOCK_METHOD3(add_available_apps,
                 void(scopes::SearchReplyProxy const&searchReply,
                      const PackageSet &locallyInstalledApps,
                      const std::string& categoryTemplate));
    MOCK_METHOD3(push_local_results, void(scopes::SearchReplyProxy const &replyProxy,
                                          std::vector<click::Application> const &apps,
                                          std::string& categoryTemplate));
    MOCK_METHOD0(get_installed_packages, PackageSet());
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

TEST(QueryTest, testAddAvailableAppsCallsClickIndex)
{
    MockIndex mock_index;
    click::DepartmentLookup dept_lookup;
    click::HighlightList highlights;
    scopes::SearchMetadata metadata("en_EN", "phone");
    PackageSet no_installed_packages;
    const unity::scopes::CannedQuery query("foo.scope", FAKE_QUERY, "");
    MockQuery q(query, mock_index, dept_lookup, highlights, metadata);
    EXPECT_CALL(mock_index, do_search(FAKE_QUERY, _)).Times(1);

    scopes::testing::MockSearchReply mock_reply;
    scopes::SearchReplyProxy reply(&mock_reply, [](unity::scopes::SearchReply*){});

    scopes::CategoryRenderer renderer("{}");
    auto ptrCat = std::make_shared<FakeCategory>("id", "", "", renderer);
    EXPECT_CALL(q, register_category(_, _, _, _, _)).Times(2).WillRepeatedly(Return(ptrCat));
    q.wrap_add_available_apps(reply, no_installed_packages, FAKE_CATEGORY_TEMPLATE);
}

TEST(QueryTest, testAddAvailableAppsPushesResults)
{
    click::Packages packages {
        {"name", "title", 0.0, "icon", "uri"}
    };
    MockIndex mock_index(packages);
    click::DepartmentLookup dept_lookup;
    click::HighlightList highlights;
    scopes::SearchMetadata metadata("en_EN", "phone");
    PackageSet no_installed_packages;
    const unity::scopes::CannedQuery query("foo.scope", FAKE_QUERY, "");
    MockQuery q(query, mock_index, dept_lookup, highlights, metadata);
    EXPECT_CALL(mock_index, do_search(FAKE_QUERY, _));

    scopes::CategoryRenderer renderer("{}");
    auto ptrCat = std::make_shared<FakeCategory>("id", "", "", renderer);
    EXPECT_CALL(q, register_category(_, _, _, _, _)).Times(2).WillRepeatedly(Return(ptrCat));

    scopes::testing::MockSearchReply mock_reply;
    scopes::SearchReplyProxy reply(&mock_reply, [](unity::scopes::SearchReply*){});

    auto expected_title = packages.front().title;
    EXPECT_CALL(q, push_result(_, Property(&scopes::CategorisedResult::title, expected_title)));
    q.wrap_add_available_apps(reply, no_installed_packages, FAKE_CATEGORY_TEMPLATE);
}

TEST(QueryTest, testAddAvailableAppsCallsFinished)
{
    click::Packages packages {
        {"name", "title", 0.0, "icon", "uri"}
    };
    MockIndex mock_index(packages);
    click::DepartmentLookup dept_lookup;
    click::HighlightList highlights;
    scopes::SearchMetadata metadata("en_EN", "phone");
    PackageSet no_installed_packages;
    const unity::scopes::CannedQuery query("foo.scope", FAKE_QUERY, "");
    MockQuery q(query, mock_index, dept_lookup, highlights, metadata);
    EXPECT_CALL(mock_index, do_search(FAKE_QUERY, _));

    scopes::CategoryRenderer renderer("{}");
    auto ptrCat = std::make_shared<FakeCategory>("id", "", "", renderer);
    EXPECT_CALL(q, register_category(_, _, _, _, _)).Times(2).WillRepeatedly(Return(ptrCat));

    scopes::testing::MockSearchReply mock_reply;
    scopes::SearchReplyProxy reply(&mock_reply, [](unity::scopes::SearchReply*){});
    EXPECT_CALL(q, finished(_));
    q.wrap_add_available_apps(reply, no_installed_packages, FAKE_CATEGORY_TEMPLATE);
}

TEST(QueryTest, testQueryRunCallsAddAvailableApps)
{
    click::Packages packages {
        {"name", "title", 0.0, "icon", "uri"}
    };
    MockIndex mock_index(packages);
    click::DepartmentLookup dept_lookup;
    click::HighlightList highlights;
    scopes::SearchMetadata metadata("en_EN", "phone");
    PackageSet no_installed_packages;
    const unity::scopes::CannedQuery query("foo.scope", FAKE_QUERY, "");
    MockQueryRun q(query, mock_index, dept_lookup, highlights, metadata);
    auto reply = scopes::SearchReplyProxy();
    EXPECT_CALL(q, get_installed_packages()).WillOnce(Return(no_installed_packages));
    EXPECT_CALL(q, add_available_apps(reply, no_installed_packages, _));

    q.run(reply);
}

MATCHER_P(HasPackageName, n, "") { return arg[click::Query::ResultKeys::NAME].get_string() == n; }
MATCHER_P(IsInstalled, b, "") { return arg[click::Query::ResultKeys::INSTALLED].get_bool() == b; }

TEST(QueryTest, testDuplicatesNotFilteredAnymore)
{
    click::Packages packages {
        {"org.example.app1", "app title1", 0.0, "icon", "uri"},
        {"org.example.app2", "app title2", 0.0, "icon", "uri"}
    };
    MockIndex mock_index(packages);
    scopes::SearchMetadata metadata("en_EN", "phone");
    PackageSet one_installed_package {
        {"org.example.app2", "0.2"}
    };
    click::DepartmentLookup dept_lookup;
    click::HighlightList highlights;
    const unity::scopes::CannedQuery query("foo.scope", FAKE_QUERY, "");
    MockQuery q(query, mock_index, dept_lookup, highlights, metadata);
    EXPECT_CALL(mock_index, do_search(FAKE_QUERY, _));

    scopes::CategoryRenderer renderer("{}");
    auto ptrCat = std::make_shared<FakeCategory>("id", "", "", renderer);
    EXPECT_CALL(q, register_category(_, _, _, _, _)).Times(2).WillRepeatedly(Return(ptrCat));

    scopes::testing::MockSearchReply mock_reply;
    scopes::SearchReplyProxy reply(&mock_reply, [](unity::scopes::SearchReply*){});
    auto expected_name1 = packages.front().name;
    EXPECT_CALL(q, push_result(_, HasPackageName(expected_name1)));
    auto expected_name2 = packages.back().name;
    EXPECT_CALL(q, push_result(_, HasPackageName(expected_name2)));
    q.wrap_add_available_apps(reply, one_installed_package, FAKE_CATEGORY_TEMPLATE);
}

TEST(QueryTest, testInstalledPackagesFlaggedAsSuch)
{
    click::Packages packages {
        {"org.example.app1", "app title1", 0.0, "icon", "uri"},
        {"org.example.app2", "app title2", 0.0, "icon", "uri"}
    };
    MockIndex mock_index(packages);
    scopes::SearchMetadata metadata("en_EN", "phone");
    PackageSet one_installed_package {
        {"org.example.app2", "0.2"}
    };
    click::DepartmentLookup dept_lookup;
    click::HighlightList highlights;
    const unity::scopes::CannedQuery query("foo.scope", FAKE_QUERY, "");
    MockQuery q(query, mock_index, dept_lookup, highlights, metadata);
    EXPECT_CALL(mock_index, do_search(FAKE_QUERY, _));

    scopes::CategoryRenderer renderer("{}");
    auto ptrCat = std::make_shared<FakeCategory>("id", "", "", renderer);
    EXPECT_CALL(q, register_category(_, _, _, _, _)).Times(2).WillRepeatedly(Return(ptrCat));

    scopes::testing::MockSearchReply mock_reply;
    scopes::SearchReplyProxy reply(&mock_reply, [](unity::scopes::SearchReply*){});
    EXPECT_CALL(q, push_result(_, IsInstalled(true)));
    EXPECT_CALL(q, push_result(_, IsInstalled(false)));
    q.wrap_add_available_apps(reply, one_installed_package, FAKE_CATEGORY_TEMPLATE);
}

class FakeInterface : public click::Interface
{
public:
    MOCK_METHOD1(get_installed_packages, void(std::function<void(PackageSet, click::InterfaceError)> callback));
};

TEST(QueryTest, testGetInstalledPackages)
{
    click::Packages uninstalled_packages {
        {"name", "title", 0.0, "icon", "uri"}
    };
    MockIndex mock_index(uninstalled_packages);
    scopes::SearchMetadata metadata("en_EN", "phone");
    click::DepartmentLookup dept_lookup;
    click::HighlightList highlights;
    const unity::scopes::CannedQuery query("foo.scope", FAKE_QUERY, "");
    MockQuery q(query, mock_index, dept_lookup, highlights, metadata);
    PackageSet installed_packages{{"package_1", "0.1"}};

    FakeInterface fake_interface;
    EXPECT_CALL(q, clickInterfaceInstance()).WillOnce(ReturnRef(fake_interface));
    EXPECT_CALL(fake_interface, get_installed_packages(_)).WillOnce(Invoke(
        [&](std::function<void(PackageSet, click::InterfaceError)> callback){
            callback(installed_packages, click::InterfaceError::NoError);
    }));

    ASSERT_EQ(q.get_installed_packages(), installed_packages);
}
