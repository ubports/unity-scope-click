/*
 * Copyright (C) 2015 Canonical Ltd.
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

#include <clickapps/apps-scope.h>
#include <click/preview.h>
#include <clickapps/apps-query.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <unity/scopes/testing/Result.h>

using namespace ::testing;

class AppsScopeTest : public Test {
protected:
    click::Scope scope;
    unity::scopes::testing::Result result;
    unity::scopes::ActionMetadata metadata;
    unity::scopes::VariantMap metadict;

public:
    AppsScopeTest() : metadata("en_EN", "phone") {
        metadict["rating"] = unity::scopes::Variant(4.0f);
        metadict["review"] = "This is a review.";
        metadata.set_scope_data(unity::scopes::Variant(metadict));
    }
};

TEST_F(AppsScopeTest, DISABLED_testConfirmUninstall)
{
    result.set_title("foo");
    result[click::apps::Query::ResultKeys::NAME] = "foo.name";
    result[click::apps::Query::ResultKeys::VERSION] = "0.1";
    auto activation = scope.perform_action(result, metadata, "widget",
                                           click::Preview::Actions::CONFIRM_UNINSTALL);
    auto response = activation->activate();
}

TEST_F(AppsScopeTest, testCancelPurchaseInstalled)
{
    auto activation = scope.perform_action(result, metadata, "button",
                                           click::Preview::Actions::CANCEL_PURCHASE_INSTALLED);
    auto response = activation->activate();
    EXPECT_TRUE(response.scope_data().get_dict()[click::Preview::Actions::CANCEL_PURCHASE_INSTALLED].get_bool());
}

TEST_F(AppsScopeTest, testCancelPurchaseUninstalled)
{
    auto activation = scope.perform_action(result, metadata, "button",
                                           click::Preview::Actions::CANCEL_PURCHASE_UNINSTALLED);
    auto response = activation->activate();
    EXPECT_TRUE(response.scope_data().get_dict()[click::Preview::Actions::CANCEL_PURCHASE_UNINSTALLED].get_bool());
}

TEST_F(AppsScopeTest, testShowInstalled)
{
    auto activation = scope.perform_action(result, metadata, "button",
                                           click::Preview::Actions::SHOW_INSTALLED);
    auto response = activation->activate();
    EXPECT_TRUE(response.scope_data().get_dict()[click::Preview::Actions::SHOW_INSTALLED].get_bool());
}

TEST_F(AppsScopeTest, testShowUninstalled)
{
    auto activation = scope.perform_action(result, metadata, "button",
                                           click::Preview::Actions::SHOW_UNINSTALLED);
    auto response = activation->activate();
    EXPECT_TRUE(response.scope_data().get_dict()[click::Preview::Actions::SHOW_UNINSTALLED].get_bool());
}

TEST_F(AppsScopeTest, testConfirmCancelPurchaseUninstalled)
{
    auto activation = scope.perform_action(result, metadata, "widget_id",
                                           click::Preview::Actions::CONFIRM_CANCEL_PURCHASE_UNINSTALLED);
    auto response = activation->activate();
    EXPECT_TRUE(response.scope_data().get_dict()[click::Preview::Actions::CONFIRM_CANCEL_PURCHASE_UNINSTALLED].get_bool());
}

TEST_F(AppsScopeTest, testConfirmCancelPurcahseInstalled)
{
    auto activation = scope.perform_action(result, metadata, "widget_id",
                                           click::Preview::Actions::CONFIRM_CANCEL_PURCHASE_INSTALLED);
    auto response = activation->activate();
    EXPECT_TRUE(response.scope_data().get_dict()[click::Preview::Actions::CONFIRM_CANCEL_PURCHASE_INSTALLED].get_bool());
}

TEST_F(AppsScopeTest, testRatingNew)
{
    auto activation = scope.perform_action(result, metadata, "rating",
                                           click::Preview::Actions::RATED);
    auto response = activation->activate();
    EXPECT_EQ("rating", response.scope_data().get_dict()["widget_id"].get_string());
}

TEST_F(AppsScopeTest, testRatingEdit)
{
    auto activation = scope.perform_action(result, metadata, "93345",
                                           click::Preview::Actions::RATED);
    auto response = activation->activate();
    EXPECT_EQ("93345", response.scope_data().get_dict()["widget_id"].get_string());
}
