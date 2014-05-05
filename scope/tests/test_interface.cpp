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

#include "fake_json.h"
#include "test_data.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QTimer>

#include <cstdlib>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <click/interface.h>
#include <click/key_file_locator.h>

using namespace click;
using namespace ::testing;

namespace
{

// TODO: Get rid of file-based testing and instead make unity::util::IniParser mockable
// Maintaining this list here will become tedious over time.
static const std::vector<click::Application> non_desktop_applications =
{
    {"com.ubuntu.developer.webapps.webapp-ubuntuone", "Ubuntu One", 0.0, "/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.developer.webapps.webapp-ubuntuone/./ubuntuone.png", "application:///com.ubuntu.developer.webapps.webapp-ubuntuone_webapp-ubuntuone_1.0.4.desktop", "", ""},
    {"com.ubuntu.stock-ticker-mobile", "Stock Ticker", 0.0, "/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.stock-ticker-mobile/icons/stock_icon_48.png", "application:///com.ubuntu.stock-ticker-mobile_stock-ticker-mobile_0.3.7.66.desktop", "", ""},
    {"", "Weather", 0.0, "/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.weather/./weather64.png", "application:///com.ubuntu.weather_weather_1.0.168.desktop", "", ""},
    {"com.ubuntu.developer.webapps.webapp-twitter", "Twitter", 0.0, "/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.developer.webapps.webapp-twitter/./twitter.png", "application:///com.ubuntu.developer.webapps.webapp-twitter_webapp-twitter_1.0.5.desktop", "", ""},
    {"com.ubuntu.music", "Music", 0.0, "/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.music/images/music.png", "application:///com.ubuntu.music_music_1.1.329.desktop", "", ""},
    {"com.ubuntu.clock", "Clock", 0.0, "/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.clock/./clock64.png", "application:///com.ubuntu.clock_clock_1.0.300.desktop", "", ""},
    {"com.ubuntu.dropping-letters", "Dropping Letters", 0.0, "/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.dropping-letters/dropping-letters.png", "application:///com.ubuntu.dropping-letters_dropping-letters_0.1.2.2.43.desktop", "", ""},
    {"com.ubuntu.developer.webapps.webapp-gmail", "Gmail", 0.0, "/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.developer.webapps.webapp-gmail/./gmail.png", "application:///com.ubuntu.developer.webapps.webapp-gmail_webapp-gmail_1.0.8.desktop", "", ""},
    {"com.ubuntu.terminal", "Terminal", 0.0, "/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.terminal/./terminal64.png", "application:///com.ubuntu.terminal_terminal_0.5.29.desktop", "", ""},
    {"com.ubuntu.calendar", "Calendar", 0.0, "/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.calendar/./calendar64.png", "application:///com.ubuntu.calendar_calendar_0.4.182.desktop", "", ""},
    {"com.ubuntu.notes", "Notes", 0.0, "image://theme/notepad", "application:///com.ubuntu.notes_notes_1.4.242.desktop", "", ""},
    {"com.ubuntu.developer.webapps.webapp-amazon", "Amazon", 0.0, "/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.developer.webapps.webapp-amazon/./amazon.png", "application:///com.ubuntu.developer.webapps.webapp-amazon_webapp-amazon_1.0.6.desktop", "", ""},
    {"com.ubuntu.shorts", "Shorts", 0.0, "/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.shorts/./rssreader64.png", "application:///com.ubuntu.shorts_shorts_0.2.162.desktop", "", ""},
    {"com.ubuntu.filemanager", "File Manager", 0.0, "/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/./filemanager64.png", "application:///com.ubuntu.filemanager_filemanager_0.1.1.97.desktop", "", ""},
    {"com.ubuntu.calculator", "Calculator", 0.0, "/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.calculator/./calculator64.png", "application:///com.ubuntu.calculator_calculator_0.1.3.206.desktop", "", ""},
    {"com.ubuntu.sudoku", "Sudoku", 0.0, "/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.sudoku/SudokuGameIcon.png", "application:///com.ubuntu.sudoku_sudoku_1.0.142.desktop", "", ""},
    {"com.ubuntu.developer.webapps.webapp-ebay", "eBay", 0.0, "/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.developer.webapps.webapp-ebay/./ebay.png", "application:///com.ubuntu.developer.webapps.webapp-ebay_webapp-ebay_1.0.8.desktop", "", ""},
    {"com.ubuntu.developer.webapps.webapp-facebook", "Facebook", 0.0, "/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.developer.webapps.webapp-facebook/./facebook.png", "application:///com.ubuntu.developer.webapps.webapp-facebook_webapp-facebook_1.0.5.desktop", "", ""},
    {"", "Messaging", 0.0, "image://theme/messages-app", "application:///messaging-app.desktop", "Messaging application", "/usr/share/messaging-app/assets/messaging-app-screenshot.png"},
    {"", "Contacts", 0.0, "image://theme/contacts-app", "application:///address-book-app.desktop", "", ""}
};

static click::Application desktop_application
{
    "",
    "Sample Desktop-only non-click app",
    0.0,
    "image://theme/sample-desktop-app",
    "application:///non-click-app-without-exception.desktop",
    "multiline description goes here",
    ""
};

}

namespace
{
const std::string emptyQuery{};

struct MockKeyFileLocator : public click::KeyFileLocator
{
    typedef click::KeyFileLocator Super;

    MockKeyFileLocator()
    {
        using namespace ::testing;

        ON_CALL(*this, enumerateKeyFilesForInstalledApplications(_))
                .WillByDefault(
                    Invoke(
                        this,
                        &MockKeyFileLocator::doEnumerateKeyFilesForInstalledApplications));
    }

    MOCK_METHOD1(enumerateKeyFilesForInstalledApplications,
                 void(const Super::Enumerator&));

    void doEnumerateKeyFilesForInstalledApplications(const Super::Enumerator& enumerator)
    {
        Super::enumerateKeyFilesForInstalledApplications(enumerator);
    }
};

class ClickInterfaceTest : public ::testing::Test {
public:
    MOCK_METHOD2(manifest_callback, void(Manifest, ManifestError));
    MOCK_METHOD2(manifests_callback, void(ManifestList, ManifestError));
};

}

class FakeClickInterface : public click::Interface {
public:
    FakeClickInterface(const QSharedPointer<KeyFileLocator>& keyFileLocator) : Interface(keyFileLocator) {}
    FakeClickInterface() {}

    MOCK_METHOD0(show_desktop_apps, bool());
    MOCK_METHOD2(run_process, void(const std::string&, std::function<void(int, const std::string&, const std::string&)>));
};

TEST(ClickInterface, testIsNonClickAppFalse)
{
    EXPECT_FALSE(Interface::is_non_click_app("unknown-app.desktop"));
}

TEST(ClickInterface, testIsNonClickAppNoRegression)
{
    // Loop through and check that all filenames are non-click filenames
    // If this ever breaks, something is very very wrong.
    for (const auto& element : nonClickDesktopFiles())
    {
        QString filename = element.c_str();
        EXPECT_TRUE(Interface::is_non_click_app(filename));
    }
}

TEST(ClickInterface, testCallsIntoKeyFileLocatorForFindingInstalledApps)
{
    using namespace ::testing;
    MockKeyFileLocator mockKeyFileLocator;
    QSharedPointer<click::KeyFileLocator> keyFileLocator(
                &mockKeyFileLocator,
                [](click::KeyFileLocator*){});


    FakeClickInterface iface(keyFileLocator);
    EXPECT_CALL(iface, show_desktop_apps())
        .WillRepeatedly(Return(false));

    EXPECT_CALL(mockKeyFileLocator, enumerateKeyFilesForInstalledApplications(_)).Times(1);

    iface.find_installed_apps(emptyQuery);
}

TEST(ClickInterface, testFindAppsInDirEmpty)
{
    QSharedPointer<click::KeyFileLocator> keyFileLocator(
                new click::KeyFileLocator(
                    testing::systemApplicationsDirectoryForTesting(),
                    testing::userApplicationsDirectoryForTesting()));

    click::Interface iface(keyFileLocator);

    auto results = iface.find_installed_apps("foo");

    EXPECT_TRUE(results.empty());
}

TEST(ClickInterface, testIsIconIdentifier)
{
    EXPECT_TRUE(Interface::is_icon_identifier("contacts-app"));
    EXPECT_FALSE(Interface::is_icon_identifier(
                    "/usr/share/unity8/graphics/applicationIcons/contacts-app@18.png"));
}

TEST(ClickInterface, testAddThemeScheme)
{
    EXPECT_EQ("image://theme/contacts-app", Interface::add_theme_scheme("contacts-app"));
    EXPECT_EQ("/usr/share/unity8/graphics/applicationIcons/contacts-app@18.png",
              Interface::add_theme_scheme("/usr/share/unity8/graphics/applicationIcons/contacts-app@18.png"));
}

std::vector<click::Application> find_installed_apps(const std::string& query, bool include_desktop_results)
{
    using namespace ::testing;
    QSharedPointer<click::KeyFileLocator> keyFileLocator(
                new click::KeyFileLocator(
                    testing::systemApplicationsDirectoryForTesting(),
                    testing::userApplicationsDirectoryForTesting()));

    FakeClickInterface iface(keyFileLocator);
    EXPECT_CALL(iface, show_desktop_apps())
        .WillRepeatedly(Return(include_desktop_results));

    return iface.find_installed_apps(query);
}

TEST(ClickInterface, testFindInstalledAppsOnPhone)
{
    auto result = find_installed_apps(emptyQuery, false);

    EXPECT_TRUE(result.size() > 0);

    for (const auto& app : non_desktop_applications)
    {
        qDebug() << "comparing" << QString::fromStdString(app.title);
        EXPECT_NE(result.end(), std::find(result.begin(), result.end(), app));
    }

    EXPECT_EQ(result.end(), std::find(result.begin(), result.end(), desktop_application));
}

TEST(ClickInterface, testFindInstalledAppsOnDesktop)
{
    auto result = find_installed_apps(emptyQuery, true);

    std::vector<click::Application> expected_apps(non_desktop_applications);
    expected_apps.push_back(desktop_application);

    EXPECT_TRUE(result.size() > 0);

    for (const auto& app : expected_apps)
    {
        qDebug() << "comparing" << QString::fromStdString(app.title);
        EXPECT_NE(result.end(), std::find(result.begin(), result.end(), app));
    }
}

TEST(ClickInterface, testIncludeInResultsNoDisplayTrue)
{
    QSharedPointer<click::KeyFileLocator> keyFileLocator(new click::KeyFileLocator());
    click::Interface iface(keyFileLocator);

    auto nodisplay = testing::userApplicationsDirectoryForTesting() + "/non-click-app-nodisplay.desktop";
    unity::util::IniParser parser(nodisplay.data());

    EXPECT_FALSE(iface.is_visible_app(parser));
}

TEST(ClickInterface, testIncludeInResultsOnlyShowInGnome)
{
    QSharedPointer<click::KeyFileLocator> keyFileLocator(new click::KeyFileLocator());
    click::Interface iface(keyFileLocator);

    auto nodisplay = testing::userApplicationsDirectoryForTesting() + "/non-click-app-onlyshowin-gnome.desktop";
    unity::util::IniParser parser(nodisplay.data());

    EXPECT_FALSE(iface.is_visible_app(parser));
}

TEST(ClickInterface, testIncludeInResultsOnlyShowInGnomeUnity)
{
    QSharedPointer<click::KeyFileLocator> keyFileLocator(new click::KeyFileLocator());
    click::Interface iface(keyFileLocator);

    auto nodisplay = testing::userApplicationsDirectoryForTesting() + "/non-click-app-onlyshowin-gnome-unity.desktop";
    unity::util::IniParser parser(nodisplay.data());

    EXPECT_TRUE(iface.is_visible_app(parser));
}

TEST(ClickInterface, testIncludeInResultsOnlyShowInUnity)
{
    QSharedPointer<click::KeyFileLocator> keyFileLocator(new click::KeyFileLocator());
    click::Interface iface(keyFileLocator);

    auto nodisplay = testing::userApplicationsDirectoryForTesting() + "/non-click-app-onlyshowin-unity.desktop";
    unity::util::IniParser parser(nodisplay.data());

    EXPECT_TRUE(iface.is_visible_app(parser));
}

TEST(ClickInterface, testEnableDesktopApps)
{
    QSharedPointer<click::KeyFileLocator> keyFileLocator(new click::KeyFileLocator());
    click::Interface iface(keyFileLocator);

    setenv(Interface::ENV_SHOW_DESKTOP_APPS, "YesPlease", true);
    EXPECT_TRUE(iface.show_desktop_apps());
}

TEST(ClickInterface, testDisableDesktopApps)
{
    QSharedPointer<click::KeyFileLocator> keyFileLocator(new click::KeyFileLocator());
    click::Interface iface(keyFileLocator);

    unsetenv(Interface::ENV_SHOW_DESKTOP_APPS);
    EXPECT_FALSE(iface.show_desktop_apps());
}

TEST(ClickInterface, testGetManifestForAppCorrectCommand)
{
    FakeClickInterface iface;
    std::string command = "click info " + FAKE_PACKAGENAME;
    EXPECT_CALL(iface, run_process(command, _)).
        Times(1);
    iface.get_manifest_for_app(FAKE_PACKAGENAME, [](Manifest, ManifestError){});
}

TEST_F(ClickInterfaceTest, testGetManifestForAppParseError)
{
    FakeClickInterface iface;
    EXPECT_CALL(iface, run_process(_, _)).
        Times(1).
        WillOnce(Invoke([&](const std::string&,
                            std::function<void(int, const std::string&,
                                               const std::string&)> callback){
                            callback(0, "INVALID JSON", "");
                        }));
    EXPECT_CALL(*this, manifest_callback(_, ManifestError::ParseError));
    iface.get_manifest_for_app(FAKE_PACKAGENAME, [this](Manifest manifest,
                                                        ManifestError error){
                                   manifest_callback(manifest, error);
                               });
}

TEST_F(ClickInterfaceTest, testGetManifestForAppCommandFailed)
{
    FakeClickInterface iface;
    EXPECT_CALL(iface, run_process(_, _)).
        Times(1).
        WillOnce(Invoke([&](const std::string&,
                            std::function<void(int, const std::string&,
                                               const std::string&)> callback){
                            callback(-1, "", "CRITICAL: FAIL");
                        }));
    EXPECT_CALL(*this, manifest_callback(_, ManifestError::CallError));
    iface.get_manifest_for_app(FAKE_PACKAGENAME, [this](Manifest manifest,
                                                        ManifestError error){
                                   manifest_callback(manifest, error);
                               });
}

TEST_F(ClickInterfaceTest, testGetManifestForAppIsRemovable)
{
    FakeClickInterface iface;
    EXPECT_CALL(iface, run_process(_, _)).
        Times(1).
        WillOnce(Invoke([&](const std::string&,
                            std::function<void(int, const std::string&,
                                               const std::string&)> callback){
                            callback(0, FAKE_JSON_MANIFEST_REMOVABLE, "");
                        }));
    iface.get_manifest_for_app(FAKE_PACKAGENAME, [](Manifest manifest,
                                                    ManifestError error){
                                   ASSERT_TRUE(error == ManifestError::NoError);
                                   ASSERT_TRUE(manifest.removable);
                               });
}

TEST_F(ClickInterfaceTest, testGetManifestForAppIsNotRemovable)
{
    FakeClickInterface iface;
    EXPECT_CALL(iface, run_process(_, _)).
        Times(1).
        WillOnce(Invoke([&](const std::string&,
                            std::function<void(int, const std::string&,
                                               const std::string&)> callback){
                            callback(0, FAKE_JSON_MANIFEST_NONREMOVABLE, "");
                        }));
    iface.get_manifest_for_app(FAKE_PACKAGENAME, [](Manifest manifest,
                                                    ManifestError error){
                                   ASSERT_TRUE(error == ManifestError::NoError);
                                   ASSERT_FALSE(manifest.removable);
                               });
}

TEST(ClickInterface, testGetManifestsCorrectCommand)
{
    FakeClickInterface iface;
    std::string command = "click list --manifest";
    EXPECT_CALL(iface, run_process(command, _)).
        Times(1);
    iface.get_manifests([](ManifestList, ManifestError){});
}

TEST_F(ClickInterfaceTest, testGetManifestsParseError)
{
    FakeClickInterface iface;
    EXPECT_CALL(iface, run_process(_, _)).
        Times(1).
        WillOnce(Invoke([&](const std::string&,
                            std::function<void(int, const std::string&,
                                               const std::string&)> callback){
                            callback(0, "INVALID JSON", "");
                        }));
    EXPECT_CALL(*this, manifests_callback(_, ManifestError::ParseError));
    iface.get_manifests([this](ManifestList manifests, ManifestError error){
            manifests_callback(manifests, error);
        });
}

TEST_F(ClickInterfaceTest, testGetManifestsCommandFailed)
{
    FakeClickInterface iface;
    EXPECT_CALL(iface, run_process(_, _)).
        Times(1).
        WillOnce(Invoke([&](const std::string&,
                            std::function<void(int, const std::string&,
                                               const std::string&)> callback){
                            callback(-1, "", "CRITICAL: FAIL");
                        }));
    EXPECT_CALL(*this, manifests_callback(_, ManifestError::CallError));
    iface.get_manifests([this](ManifestList manifests, ManifestError error){
            manifests_callback(manifests, error);
        });
}

TEST_F(ClickInterfaceTest, testGetManifestsParsed)
{
    FakeClickInterface iface;
    std::string expected_str = "[" + FAKE_JSON_MANIFEST_NONREMOVABLE + "," +
        FAKE_JSON_MANIFEST_REMOVABLE + "]";
    ManifestList expected = manifest_list_from_json(expected_str);

    EXPECT_CALL(iface, run_process(_, _)).
        Times(1).
        WillOnce(Invoke([&](const std::string&,
                            std::function<void(int, const std::string&,
                                               const std::string&)> callback){
                            callback(0, expected_str, "");
                        }));
    iface.get_manifests([expected](ManifestList manifests, ManifestError error){
            ASSERT_TRUE(error == ManifestError::NoError);
            ASSERT_TRUE(manifests.size() == expected.size());
        });
}
