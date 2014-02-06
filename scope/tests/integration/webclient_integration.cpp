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

#include <click/network_access_manager.h>
#include <click/webclient.h>
#include <click/index.h>

#include <QCoreApplication>
#include <QDebug>
#include <QThread>
#include <QTimer>

#include <gtest/gtest.h>

namespace
{
struct IntegrationTest : public ::testing::Test
{
    IntegrationTest() : app(argc, argv)
    {
        QObject::connect(
                    &testTimeout, &QTimer::timeout,
                    [this]() { FAIL() << "Operation timed out."; });
    }

    void SetUp()
    {
        const int tenSeconds = 10 * 1000;
        testTimeout.start(tenSeconds);
    }

    void Quit()
    {
        app.quit();
    }

    int argc = 0;
    char** argv = nullptr;
    QCoreApplication app;
    QTimer testTimeout;
};

}

TEST_F(IntegrationTest, queryForArmhfPackagesReturnsCorrectResults)
{
    click::web::Service ws(click::SEARCH_BASE_URL,
                           QSharedPointer<click::network::AccessManager>(
                               new click::network::AccessManager()));

    click::web::CallParams params;
    params.add("q", "qr,architecture:armhf");
    auto wr = ws.call(click::SEARCH_PATH, params);

    QObject::connect(
                wr.data(), &click::web::Response::finished,
                [this](QString s) { EXPECT_TRUE(s.size() > 0); Quit(); });

    app.exec();
}

TEST_F(IntegrationTest, DISABLED_queryForArmhfPackagesCanBeParsed)
{
    QSharedPointer<click::network::AccessManager> namPtr(
                new click::network::AccessManager());
    click::web::Service service(click::SEARCH_BASE_URL, namPtr);
    QSharedPointer<click::web::Service> servicePtr(&service);
    click::Index index(servicePtr);
    index.search("qr,architecture:armhf", [&](click::PackageList packages){
        EXPECT_TRUE(packages.size() > 0);
        Quit();
    });
    app.exec();
}

TEST_F(IntegrationTest, DISABLED_detailsCanBeParsed)
{
    const std::string sample_name("ar.com.beuno.wheather-touch");
    QSharedPointer<click::network::AccessManager> namPtr(
                new click::network::AccessManager());
    click::web::Service service(click::SEARCH_BASE_URL, namPtr);
    QSharedPointer<click::web::Service> servicePtr(&service);
    click::Index index(servicePtr);
    index.get_details(sample_name, [&](click::PackageDetails details){
        EXPECT_EQ(details.name, sample_name);
        Quit();
    });
    app.exec();
}
