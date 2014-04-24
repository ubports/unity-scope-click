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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <click/configuration.h>

using namespace click;

namespace
{

class FakeConfiguration : public click::Configuration
{
public:
    MOCK_METHOD2(list_folder, std::vector<std::string>(
                     const std::string& folder, const std::string& pattern));
};

}


TEST(Configuration, getAvailableFrameworksUsesRightFolder)
{
    using namespace ::testing;
    FakeConfiguration locator;
    EXPECT_CALL(locator, list_folder(Configuration::FRAMEWORKS_FOLDER, _))
            .Times(1).WillOnce(Return(std::vector<std::string>()));
    locator.get_available_frameworks();
}

TEST(Configuration, getAvailableFrameworksUsesRightPattern)
{
    using namespace ::testing;
    FakeConfiguration locator;
    EXPECT_CALL(locator, list_folder(_, Configuration::FRAMEWORKS_PATTERN))
            .Times(1).WillOnce(Return(std::vector<std::string>()));
    locator.get_available_frameworks();
}

TEST(Configuration, getAvailableFrameworksTwoResults)
{
    using namespace ::testing;

    FakeConfiguration locator;
    std::vector<std::string> response = {"abc.framework", "def.framework"};
    EXPECT_CALL(locator, list_folder(_, _))
            .Times(1)
            .WillOnce(Return(response));
    auto frameworks = locator.get_available_frameworks();
    std::vector<std::string> expected = {"abc", "def"};
    EXPECT_EQ(expected, frameworks);
}

TEST(Configuration, getAvailableFrameworksNoResults)
{
    using namespace ::testing;

    FakeConfiguration locator;
    std::vector<std::string> response = {};
    EXPECT_CALL(locator, list_folder(_, _))
            .Times(1)
            .WillOnce(Return(response));
    auto frameworks = locator.get_available_frameworks();
    EXPECT_EQ(0, frameworks.size());
}
