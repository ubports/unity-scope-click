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

#include <unity/scopes/SearchReply.h>
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

MATCHER_P(HasApplicationTitle, n, "") { return arg["title"].get_string() == n; }

TEST_F(ResultPusherTest, testPushTopAndLocalResults)
{
    std::string categoryTemplate("{}");
    std::vector<click::Application> apps {
        {"app1", "App1", 0.0f, "icon", "url", "", "sshot"},
        {"app2", "App2", 0.0f, "icon", "url", "", "sshot"},
        {"app3", "App3", 0.0f, "icon", "url", "", "sshot"},
        {"", "App4", 0.0f, "icon", "application:///app4.desktop", "", "sshot"} // a non-click app
    };

    click::apps::ResultPusher pusher(reply, {"app2", "app4"});
    auto mockreply = (scopes::testing::MockSearchReply*)reply.get();

    scopes::CategoryRenderer renderer("{}");
    auto ptrCat = std::make_shared<FakeCategory>("id", "", "", renderer);

    EXPECT_CALL(*mockreply, register_category(_, _, _, _)).WillRepeatedly(Return(ptrCat));
    EXPECT_CALL(*mockreply, push(Matcher<unity::scopes::CategorisedResult const&>(HasApplicationTitle(std::string("App2")))));
    EXPECT_CALL(*mockreply, push(Matcher<unity::scopes::CategorisedResult const&>(HasApplicationTitle(std::string("App4")))));

    EXPECT_CALL(*mockreply, push(Matcher<unity::scopes::CategorisedResult const&>(HasApplicationTitle(std::string("App1")))));
    EXPECT_CALL(*mockreply, push(Matcher<unity::scopes::CategorisedResult const&>(HasApplicationTitle(std::string("App3")))));
    pusher.push_top_results(apps, categoryTemplate);
    pusher.push_local_results(apps, categoryTemplate);
}

