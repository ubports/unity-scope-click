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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <unity/scopes/testing/Result.h>

#include <click/preview.h>
#include <clickstore/store-scope.h>

using namespace ::testing;

class StoreScopeTest : public Test {
protected:
    const std::string FAKE_SHA512 = "FAKE_SHA512";
    click::Scope scope;
    unity::scopes::testing::Result result;
    unity::scopes::ActionMetadata metadata;
    unity::scopes::VariantMap metadict;

public:
    StoreScopeTest() : metadata("en_EN", "phone") {
        metadict["download_url"] = "fake_download_url";
        metadict["download_sha512"] = FAKE_SHA512;
        metadata.set_scope_data(unity::scopes::Variant(metadict));
    }
};

TEST_F(StoreScopeTest, testPurchaseCompletedPassesHash)
{
    auto activation = scope.perform_action(result, metadata, "widget_id", "purchaseCompleted");
    auto response = activation->activate();
    EXPECT_EQ(FAKE_SHA512, response.scope_data().get_dict()["download_sha512"].get_string());
}

TEST_F(StoreScopeTest, testInstallClickPassesHash)
{
    auto activation = scope.perform_action(result, metadata, "widget_id", click::Preview::Actions::INSTALL_CLICK);
    auto response = activation->activate();
    EXPECT_EQ(FAKE_SHA512, response.scope_data().get_dict()["download_sha512"].get_string());
}
