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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <unity/scopes/testing/Result.h>

#include <click/preview.h>
#include <clickapps/apps-scope.h>

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

TEST_F(AppsScopeTest, testStoreScopeRatingNew)
{
    auto activation = scope.perform_action(result, metadata, "rating", click::Preview::Actions::RATED);
    auto response = activation->activate();
    EXPECT_EQ("rating", response.scope_data().get_dict()["widget_id"].get_string());
}

TEST_F(AppsScopeTest, testStoreScopeRatingEdit)
{
    auto activation = scope.perform_action(result, metadata, "93345", click::Preview::Actions::RATED);
    auto response = activation->activate();
    EXPECT_EQ("93345", response.scope_data().get_dict()["widget_id"].get_string());
}
