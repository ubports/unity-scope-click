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
#include <QTimer>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <click/interface.h>

using namespace click;


struct Fixture : public ::testing::Test
{
public:
    void Quit() {
    };

    QCoreApplication app;
};

struct InterfaceSignalSpy
{
    MOCK_METHOD1(on_installed_apps_found, void(QList<Application>&));
};

/* WTF THIS DOES NOT WORK
TEST_F(Fixture, testFindInstalledApps)
{
    Interface iface;
    InterfaceSignalSpy spy;

    QObject::connect(&iface, &Interface::installed_apps_found,
                     [&spy]() {
                         spy.on_installed_apps_found(QList<Application>&);
                     } );

    EXPECT_CALL(spy, on_installed_apps_found(::testing::_)).Times(1)
        .WillOnce(InvokeWithoutArgs(this, &Fixture::Quit));

    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, [&]() {
            Interface::find_installed_apps(QStringLiteral("foo"));
        } );
    timer.start(0);
}
*/
