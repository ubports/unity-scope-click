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

#include <gio/gio.h>
#include <libdbustest/dbus-test.h>

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

    // For mocking the pay-service dbus API
    DbusTestService* service = nullptr;
    DbusTestDbusMock* mock = nullptr;
    DbusTestDbusMockObject* pkgobj = nullptr;

    virtual void SetUp()
    {
        // Mock the dbus interfaces from pay-service
        service = dbus_test_service_new(nullptr);

        mock = dbus_test_dbus_mock_new("com.canonical.pay");

        pkgobj = dbus_test_dbus_mock_get_object(mock, "/com/canonical/pay/click_2Dscope", "com.canonical.pay.package", nullptr);

        dbus_test_dbus_mock_object_add_method(mock, pkgobj,
                                              "VerifyItem",
                                              G_VARIANT_TYPE_STRING,
                                              nullptr, /* out */
                                              "", /* python */
                                              nullptr); /* error */

        dbus_test_dbus_mock_object_add_method(mock, pkgobj,
                                              "PurchaseItem",
                                              G_VARIANT_TYPE_STRING,
                                              nullptr, /* out */
                                              "", /* python */
                                              nullptr); /* error */

        dbus_test_dbus_mock_object_add_method(mock, pkgobj,
                                              "RefundItem",
                                              G_VARIANT_TYPE_STRING,
                                              nullptr, /* out */
                                              "", /* python */
                                              nullptr); /* error */

        dbus_test_service_add_task(service, DBUS_TEST_TASK(mock));

        dbus_test_service_start_tasks(service);

        namPtr.reset(new MockNetworkAccessManager());
        clientPtr.reset(new NiceMock<MockClient>(namPtr));
        package.reset(new MockPayPackage(clientPtr));
    }

    virtual void TearDown()
    {
        g_clear_object(&mock);
        g_clear_object(&service);
    }

public:
    MOCK_METHOD1(purchases_callback, void(pay::PurchaseSet));
};

}

TEST_F(PayTest, testIsRefundableTrue)
{
    GError* error = nullptr;

    dbus_test_dbus_mock_object_emit_signal(mock, pkgobj,
                                           "ItemStatusChanged",
                                           G_VARIANT_TYPE("(sst)"),
                                           g_variant_new("(sst)",
                                                         "item", "purchased",
                                                         (guint64) G_MAXINT64),
                                           &error);
    // No Hondas here, why racing?
    usleep(10000);

    ASSERT_EQ(nullptr, error);
    ASSERT_TRUE(package->is_refundable("item"));
}

TEST_F(PayTest, testIsRefundableFalse)
{
    GError* error = nullptr;

    dbus_test_dbus_mock_object_emit_signal(mock, pkgobj,
                                           "ItemStatusChanged",
                                           G_VARIANT_TYPE("(sst)"),
                                           g_variant_new("(sst)",
                                                         "item", "purchasing",
                                                         (guint64) 0),
                                           &error);
    usleep(10000);

    ASSERT_EQ(nullptr, error);
    ASSERT_FALSE(package->is_refundable("item"));
}

TEST_F(PayTest, testPayPackageRefundCalled)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(*package, do_pay_package_refund("foo")).Times(1);
    EXPECT_EQ(false, package->refund("foo"));
}

TEST_F(PayTest, testPayPackageRefundNotCalledIfCallbackExists)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    std::string callback_id = std::string{"foo"} + pay::APPENDAGE_REFUND;
    package->callbacks[callback_id] = [](const std::string&, bool) {};
    EXPECT_CALL(*package, do_pay_package_refund("foo")).Times(0);
    EXPECT_EQ(false, package->refund("foo"));
}

TEST_F(PayTest, testRefundReturnsTrueForPurchasedItem)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    package->success = true;
    EXPECT_CALL(*package, do_pay_package_refund("foo")).Times(1);
    EXPECT_EQ(true, package->refund("foo"));
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

    std::string callback_id = std::string{"foo"} + pay::APPENDAGE_VERIFY;
    package->callbacks[callback_id] = [](const std::string&, bool) {};
    EXPECT_CALL(*package, do_pay_package_verify("foo")).Times(0);
    EXPECT_EQ(false, package->verify("foo"));
}

TEST_F(PayTest, testVerifyReturnsTrueForPurchasedItem)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    package->success = true;
    EXPECT_CALL(*package, do_pay_package_verify("foo")).Times(1);
    EXPECT_EQ(true, package->verify("foo"));
}

TEST_F(PayTest, testGetPurchasesCallsWebservice)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));

    package->get_purchases([](pay::PurchaseSet) {});
}

TEST_F(PayTest, testGetPurchasesSendsCorrectPath)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(*clientPtr, callImpl(EndsWith(pay::PURCHASES_API_PATH),
                                     _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));

    package->get_purchases([](pay::PurchaseSet) {});
}

TEST_F(PayTest, testGetPurchasesCallbackCalled)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    QByteArray fake_json("[]");
    EXPECT_CALL(reply.instance, readAll())
            .Times(1)
            .WillOnce(Return(fake_json));
    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));
    EXPECT_CALL(*this, purchases_callback(_)).Times(1);

    package->get_purchases([this](pay::PurchaseSet purchases) {
            purchases_callback(purchases);
        });
    response->replyFinished();
}

TEST_F(PayTest, testGetPurchasesEmptyJsonIsParsed)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    QByteArray fake_json("[]");
    EXPECT_CALL(reply.instance, readAll())
            .Times(1)
            .WillOnce(Return(fake_json));
    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));
    pay::PurchaseSet empty_purchases_list;
    EXPECT_CALL(*this, purchases_callback(empty_purchases_list)).Times(1);

    package->get_purchases([this](pay::PurchaseSet purchases) {
            purchases_callback(purchases);
        });
    response->replyFinished();
}

TEST_F(PayTest, testGetPurchasesSingleJsonIsParsedNullTimestamp)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    QByteArray fake_json(FAKE_PURCHASES_LIST_JSON_NULL_TIMESTAMP);
    EXPECT_CALL(reply.instance, readAll())
            .Times(1)
            .WillOnce(Return(fake_json));
    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));
    pay::PurchaseSet single_purchase_list{{"com.example.fake", 0}};
    EXPECT_CALL(*this, purchases_callback(single_purchase_list)).Times(1);

    package->get_purchases([this](pay::PurchaseSet purchases) {
            purchases_callback(purchases);
        });
    response->replyFinished();
}

TEST_F(PayTest, testGetPurchasesTimestampIsParsed)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    QByteArray fake_json(FAKE_PURCHASES_LIST_JSON);
    EXPECT_CALL(reply.instance, readAll())
            .Times(1)
            .WillOnce(Return(fake_json));
    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));
    const time_t EIGHTYTHREE_SECONDS_INTO_THE_SEVENTIES=83;
    pay::PurchaseSet single_purchase_list{{"com.example.fake", EIGHTYTHREE_SECONDS_INTO_THE_SEVENTIES}};
    EXPECT_CALL(*this, purchases_callback(single_purchase_list)).Times(1);

    package->get_purchases([this](pay::PurchaseSet purchases) {
            purchases_callback(purchases);
        });
    response->replyFinished();
}

TEST_F(PayTest, testGetPurchasesIsCancellable)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));

    auto get_purchases_op = package->get_purchases([](pay::PurchaseSet) {});
    EXPECT_CALL(reply.instance, abort()).Times(1);
    get_purchases_op.cancel();
}

TEST_F(PayTest, testGetBaseUrl)
{
    const char *value = getenv(pay::BASE_URL_ENVVAR);
    if (value != NULL) {
        ASSERT_TRUE(unsetenv(pay::BASE_URL_ENVVAR) == 0);
    }
    ASSERT_TRUE(pay::Package::get_base_url() == pay::BASE_URL);
    
}

TEST_F(PayTest, testGetBaseUrlFromEnv)
{
    ASSERT_TRUE(setenv(pay::BASE_URL_ENVVAR, FAKE_SERVER.c_str(), 1) == 0);
    ASSERT_TRUE(pay::Package::get_base_url() == FAKE_SERVER);
    ASSERT_TRUE(unsetenv(pay::BASE_URL_ENVVAR) == 0);
}
