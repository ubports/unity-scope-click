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
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QTimer>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <click/interface.h>

using namespace click;


struct Fixture : public ::testing::Test
{
public:
    Fixture()
        : app(argc, argv)
    {
        signalTimer.setSingleShot(true);
        testTimeout.setSingleShot(true);

        QObject::connect(
                    &testTimeout, &QTimer::timeout,
                    [this]() { app.quit(); FAIL() << "Operation timed out."; } );
    }

    void SetUp()
    {
        const int oneSecondInMsec = 1000;
        testTimeout.start(10 * oneSecondInMsec);
    }

    void Quit() {
        app.quit();
    };

    int argc = 0;
    char** argv = nullptr;
    QCoreApplication app;
    QTimer testTimeout;
    QTimer signalTimer;
};

struct InterfaceSignalSpy
{
    MOCK_METHOD1(on_installed_apps_found, void(std::list<Application>));
};

TEST_F(Fixture, testFindInstalledApps)
{
    Interface iface;
    InterfaceSignalSpy spy;

    QObject::connect(&iface, &Interface::installed_apps_found,
                     [&spy](std::list<Application>& apps) {
                         spy.on_installed_apps_found(apps);
                     } );

    EXPECT_CALL(spy, on_installed_apps_found(::testing::_)).Times(1)
        .WillOnce(InvokeWithoutArgs(this, &Fixture::Quit));

    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, [&]() {
            iface.find_installed_apps("");
        } );
    timer.start(0);

    app.exec();
}

TEST_F(Fixture, testIsNonClickAppFalse)
{
    EXPECT_FALSE(Interface::is_non_click_app("unknown-app.desktop"));
}

TEST_F(Fixture, testIsNonClickAppNoRegression)
{
    std::list<std::string>::const_iterator iter;
    // Loop through and check that all filenames are non-click filenames
    // If this ever breaks, something is very very wrong.
    for (iter = NON_CLICK_DESKTOPS.begin();
         iter != NON_CLICK_DESKTOPS.end(); iter++) {
        QString filename = (*iter).c_str();
        EXPECT_TRUE(Interface::is_non_click_app(filename));
    }
}

TEST_F(Fixture, testFindAppsInDirEmpty)
{
    std::list<Application> results;

    Interface::find_apps_in_dir(QDir::currentPath(), "foo", results);
    EXPECT_TRUE(results.empty());
}
