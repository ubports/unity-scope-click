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

#include <click/package.h>

#include <gtest/gtest.h>

#include "fake_json.h"

using namespace click;


class PackageTest : public ::testing::Test {
};

TEST_F(PackageTest, testPackageListFromJsonNodeSingle)
{
    Json::Value root;
    Json::Reader().parse(FAKE_JSON_SEARCH_RESULT_ONE, root);
    auto const embedded = root[Package::JsonKeys::embedded];
    auto const ci_package = embedded[Package::JsonKeys::ci_package];

    Packages pl = package_list_from_json_node(ci_package);
    ASSERT_EQ(1, pl.size());
}

TEST_F(PackageTest, testPackageListFromJsonNodeSingleScope)
{
    Json::Value root;
    Json::Reader().parse(FAKE_JSON_SEARCH_RESULT_ONE_SCOPE, root);
    auto const embedded = root[Package::JsonKeys::embedded];
    auto const ci_package = embedded[Package::JsonKeys::ci_package];

    Packages pl = package_list_from_json_node(ci_package);
    ASSERT_EQ(1, pl.size());
    EXPECT_EQ("scope", pl[0].content);
}

TEST_F(PackageTest, testPackageListFromJsonNodeMany)
{
    Json::Value root;
    Json::Reader().parse(FAKE_JSON_SEARCH_RESULT_MANY, root);
    auto const embedded = root[Package::JsonKeys::embedded];
    auto const ci_package = embedded[Package::JsonKeys::ci_package];

    Packages pl = package_list_from_json_node(ci_package);
    ASSERT_GT(pl.size(), 1);
}

TEST_F(PackageTest, testPackageListFromJsonNodeMissingData)
{
    Json::Value root;
    Json::Reader().parse(FAKE_JSON_SEARCH_RESULT_MISSING_DATA, root);
    auto const embedded = root[Package::JsonKeys::embedded];
    auto const ci_package = embedded[Package::JsonKeys::ci_package];

    Packages pl = package_list_from_json_node(ci_package);
    ASSERT_EQ(1, pl.size());
}

TEST_F(PackageTest, testPackageParsesMultiplePrices)
{
    Json::Value root;
    Json::Reader().parse(FAKE_JSON_SEARCH_RESULT_ONE, root);
    auto const embedded = root[Package::JsonKeys::embedded];
    auto const ci_package = embedded[Package::JsonKeys::ci_package];

    Packages pl = package_list_from_json_node(ci_package);
    ASSERT_EQ(3, pl[0].prices.size());
}
