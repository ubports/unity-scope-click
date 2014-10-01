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

#include <QStringList>

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
    MOCK_CONST_METHOD2(get_dconf_strings, const std::vector<std::string>(const std::string& schema, const std::string& key));
    using Configuration::get_default_core_apps;
};

}

TEST(Configuration, getCoreAppsFound)
{
    using namespace ::testing;
    FakeConfiguration c;
    EXPECT_CALL(c, get_dconf_strings(Configuration::COREAPPS_SCHEMA,
                                     Configuration::COREAPPS_KEY))
            .WillOnce(Return(std::vector<std::string>{"package1", "package2"}));
    auto found_apps = c.get_core_apps();
    auto expected_apps = std::vector<std::string>{"package1", "package2"};
    ASSERT_EQ(found_apps, expected_apps);
}

TEST(Configuration, getCoreAppsEmpty)
{
    using namespace ::testing;
    FakeConfiguration c;
    EXPECT_CALL(c, get_dconf_strings(Configuration::COREAPPS_SCHEMA,
                                     Configuration::COREAPPS_KEY))
            .WillOnce(Return(std::vector<std::string>{}));
    auto found_apps = c.get_core_apps();
    auto expected_apps = c.get_default_core_apps();
    ASSERT_EQ(found_apps, expected_apps);
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

TEST(Configuration, getLanguageCorrect)
{
    ASSERT_EQ(setenv(Configuration::LANGUAGE_ENVVAR, "en_US.UTF-8", 1), 0);
    EXPECT_EQ(Configuration().get_language(), "en_US");
    ASSERT_EQ(unsetenv(Configuration::LANGUAGE_ENVVAR), 0);
}

TEST(Configuration, getLanguageUnsetFallback)
{
    ASSERT_EQ(unsetenv(Configuration::LANGUAGE_ENVVAR), 0);
    ASSERT_EQ(Configuration().get_language(), "C");
}

TEST(Configuration, getLanguageNoCharsetCorrect)
{
    ASSERT_EQ(setenv(Configuration::LANGUAGE_ENVVAR, "en_US", 1), 0);
    EXPECT_EQ(Configuration().get_language(), "en_US");
    ASSERT_EQ(unsetenv(Configuration::LANGUAGE_ENVVAR), 0);
}

TEST(Configuration, getLanguageNoRegionOrCharsetCorrect)
{
    ASSERT_EQ(setenv(Configuration::LANGUAGE_ENVVAR, "en", 1), 0);
    EXPECT_EQ(Configuration().get_language(), "en");
    ASSERT_EQ(unsetenv(Configuration::LANGUAGE_ENVVAR), 0);
}

TEST(Configuration, getLanguageBaseCorrect)
{
    ASSERT_EQ(setenv(Configuration::LANGUAGE_ENVVAR, "en_US.UTF-8", 1), 0);
    EXPECT_EQ(Configuration().get_language_base(), "en");
    ASSERT_EQ(unsetenv(Configuration::LANGUAGE_ENVVAR), 0);
}

TEST(Configuration, getLanguageBaseUnsetFallback)
{
    ASSERT_EQ(unsetenv(Configuration::LANGUAGE_ENVVAR), 0);
    ASSERT_EQ(Configuration().get_language_base(), "C");
}

TEST(Configuration, getLanguageBaseNoCharseteCorrect)
{
    ASSERT_EQ(setenv(Configuration::LANGUAGE_ENVVAR, "en_US", 1), 0);
    EXPECT_EQ(Configuration().get_language_base(), "en");
    ASSERT_EQ(unsetenv(Configuration::LANGUAGE_ENVVAR), 0);
}

TEST(Configuration, getLanguageBaseNoRegionOrCharseteCorrect)
{
    ASSERT_EQ(setenv(Configuration::LANGUAGE_ENVVAR, "en", 1), 0);
    EXPECT_EQ(Configuration().get_language_base(), "en");
    ASSERT_EQ(unsetenv(Configuration::LANGUAGE_ENVVAR), 0);
}

TEST(Configuration, getAcceptLanguagesCorrect)
{
    ASSERT_EQ(setenv(Configuration::LANGUAGE_ENVVAR, "en_US.UTF-8", 1), 0);
    EXPECT_EQ(Configuration().get_accept_languages(), "en-US, en");
    ASSERT_EQ(unsetenv(Configuration::LANGUAGE_ENVVAR), 0);
}

TEST(Configuration, getAcceptLanguagesUnsetFallback)
{
    ASSERT_EQ(unsetenv(Configuration::LANGUAGE_ENVVAR), 0);
    ASSERT_EQ(Configuration().get_accept_languages(), "C");
}

TEST(Configuration, getAcceptLanguagesNoCharseteCorrect)
{
    ASSERT_EQ(setenv(Configuration::LANGUAGE_ENVVAR, "en_US", 1), 0);
    EXPECT_EQ(Configuration().get_accept_languages(), "en-US, en");
    ASSERT_EQ(unsetenv(Configuration::LANGUAGE_ENVVAR), 0);
}

TEST(Configuration, getAcceptLanguagesNoRegionOrCharseteCorrect)
{
    ASSERT_EQ(setenv(Configuration::LANGUAGE_ENVVAR, "en", 1), 0);
    EXPECT_EQ(Configuration().get_accept_languages(), "en");
    ASSERT_EQ(unsetenv(Configuration::LANGUAGE_ENVVAR), 0);
}

TEST(Configuration, isFullLangCodeTestAllFullLangCodes)
{
    for (auto lang: Configuration::FULL_LANG_CODES) {
        EXPECT_TRUE(Configuration::is_full_lang_code(lang));
    }
}

TEST(Configuration, isFullLangCodeReturnsFalseForEN)
{    
    ASSERT_FALSE(Configuration::is_full_lang_code("en_US"));
}

TEST(Configuration, getArchitectureOverride)
{
    ASSERT_EQ(setenv(Configuration::ARCH_ENVVAR, "otherarch", 1), 0);
    EXPECT_EQ("otherarch", Configuration().get_architecture());
    ASSERT_EQ(unsetenv(Configuration::ARCH_ENVVAR), 0);
}

TEST(Configuration, getArchitectureSystem)
{
    ASSERT_EQ(unsetenv(Configuration::ARCH_ENVVAR), 0);
    ASSERT_NE("otherarch", Configuration().get_architecture());
}

TEST(Configuration, getPurchasesEnabledOverrideTrue)
{
    ASSERT_EQ(setenv(Configuration::PURCHASES_ENVVAR, "1", 1), 0);
    EXPECT_EQ(true, Configuration().get_purchases_enabled());
    ASSERT_EQ(unsetenv(Configuration::PURCHASES_ENVVAR), 0);
}

TEST(Configuration, getPurchasesEnabledOverrideFalse)
{
    ASSERT_EQ(setenv(Configuration::PURCHASES_ENVVAR, "0", 1), 0);
    EXPECT_EQ(false, Configuration().get_purchases_enabled());
    ASSERT_EQ(unsetenv(Configuration::PURCHASES_ENVVAR), 0);
}

TEST(Configuration, getPurchasesEnabledDefault)
{
    ASSERT_EQ(unsetenv(Configuration::PURCHASES_ENVVAR), 0);
    ASSERT_EQ(false, Configuration().get_purchases_enabled());
}

TEST(Configuration, getCurrencyDefault)
{
    ASSERT_EQ(unsetenv(Configuration::CURRENCY_ENVVAR), 0);
    EXPECT_EQ("USD", Configuration().get_currency());
}

TEST(Configuration, getCurrencyFallback)
{
    ASSERT_EQ(unsetenv(Configuration::CURRENCY_ENVVAR), 0);
    EXPECT_EQ("HKD", Configuration().get_currency("HKD"));
}

TEST(Configuration, getCurrencyFallbackUnknown)
{
    ASSERT_EQ(unsetenv(Configuration::CURRENCY_ENVVAR), 0);
    EXPECT_EQ("USD", Configuration().get_currency("not_valid"));
}

TEST(Configuration, getCurrencyOverride)
{
    ASSERT_EQ(setenv(Configuration::CURRENCY_ENVVAR, "TWD", 1), 0);
    EXPECT_EQ("TWD", Configuration().get_currency());
    ASSERT_EQ(unsetenv(Configuration::CURRENCY_ENVVAR), 0);
}

TEST(Configuration, getCurrencyOverrideUnknown)
{
    ASSERT_EQ(setenv(Configuration::CURRENCY_ENVVAR, "not_valid", 1), 0);
    EXPECT_EQ("USD", Configuration().get_currency());
    ASSERT_EQ(unsetenv(Configuration::CURRENCY_ENVVAR), 0);
}
