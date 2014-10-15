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

#include "mock_pay.h"

#include <click/webclient.h>

#include <tests/mock_network_access_manager.h>
#include <tests/mock_ubuntuone_credentials.h>
#include <tests/mock_webclient.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>

using namespace ::testing;


namespace
{

class PayTest : public ::testing::Test
{
protected:
    QSharedPointer<MockNetworkAccessManager> namPtr;
    QSharedPointer<MockClient> clientPtr;
    std::shared_ptr<MockPayPackage> package;

    virtual void SetUp()
    {
        namPtr.reset(new MockNetworkAccessManager());
        clientPtr.reset(new NiceMock<MockClient>(namPtr));
        package.reset(new MockPayPackage(clientPtr));
    }
};

}

TEST_F(PayTest, testPayPackageVerifyCalled)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(*package, do_pay_package_verify("foo")).Times(1);
    EXPECT_EQ(false, package->verify("foo"));
}

TEST_F(PayTest, testPayPackageVerifyNotCalledIfCallbackExists)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    package->callbacks["foo"] = [](const std::string&, bool) {};
    EXPECT_CALL(*package, do_pay_package_verify("foo")).Times(0);
    EXPECT_EQ(false, package->verify("foo"));
}

TEST_F(PayTest, testVerifyReturnsTrueForPurchasedItem)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    package->purchased = true;
    EXPECT_CALL(*package, do_pay_package_verify("foo")).Times(1);
    EXPECT_EQ(true, package->verify("foo"));
}
