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
#include <click/departments.h>

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
                    [this]() { Quit(); FAIL() << "Operation timed out."; });
    }

    void SetUp()
    {
        const int tenSeconds = 10 * 1000;
        testTimeout.start(tenSeconds);
    }

    void TearDown()
    {
        testTimeout.stop();
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
    click::web::Client ws(QSharedPointer<click::network::AccessManager>(
                              new click::network::AccessManager()));

    click::web::CallParams params;
    params.add("q", "qr,architecture:armhf");
    auto wr = ws.call(click::SEARCH_BASE_URL + click::SEARCH_PATH, params);

    QString content;
    QObject::connect(
                wr.data(), &click::web::Response::finished,
                [&, this](QString found_content) {
                    content = found_content;
                    Quit();
                });

    app.exec();
    EXPECT_TRUE(content.size() > 0);
}

TEST_F(IntegrationTest, queryForArmhfPackagesCanBeParsed)
{
    QSharedPointer<click::network::AccessManager> namPtr(
                new click::network::AccessManager());
    QSharedPointer<click::web::Client> clientPtr(
                new click::web::Client(namPtr));
    click::Index index(clientPtr);
    click::PackageList packages;
    index.search("qr,architecture:armhf", "", [&, this](click::PackageList found_packages, click::DepartmentList){
        //TODO departments
        packages = found_packages;
        Quit();
    });
    app.exec();
    EXPECT_TRUE(packages.size() > 0);
}

TEST_F(IntegrationTest, detailsCanBeParsed)
{
    const std::string sample_name("com.ubuntu.developer.alecu.qr-code");
    QSharedPointer<click::network::AccessManager> namPtr(
                new click::network::AccessManager());
    QSharedPointer<click::web::Client> clientPtr(
                new click::web::Client(namPtr));
    click::Index index(clientPtr);
    index.get_details(sample_name, [&](click::PackageDetails details, click::Index::Error){
        EXPECT_EQ(details.package.name, sample_name);
        Quit();
    });
    app.exec();
}
