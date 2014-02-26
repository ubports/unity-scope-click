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

#include "click/index.h"
#include "click/webclient.h"

#include "mock_network_access_manager.h"
#include "mock_ubuntuone_credentials.h"
#include "mock_webservice.h"
#include "fake_json.h"

#include <gtest/gtest.h>
#include <memory>

using namespace ::testing;

namespace
{

class IndexTest : public ::testing::Test {
protected:
    QSharedPointer<MockService> servicePtr;
    QSharedPointer<MockNetworkAccessManager> namPtr;
    QSharedPointer<MockCredentialsService> ssoPtr;
    std::shared_ptr<click::Index> indexPtr;

    virtual void SetUp() {
        namPtr.reset(new MockNetworkAccessManager());
        ssoPtr.reset(new MockCredentialsService());
        servicePtr.reset(new NiceMock<MockService>(namPtr, ssoPtr));
        indexPtr.reset(new click::Index(servicePtr));
    }

public:
    MOCK_METHOD1(search_callback, void(click::PackageList));
    MOCK_METHOD1(details_callback, void(click::PackageDetails));
};
}

TEST_F(IndexTest, testSearchCallsWebservice)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(*servicePtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));

    indexPtr->search("", [](click::PackageList) {});
}

TEST_F(IndexTest, testSearchSendsQueryAsParam)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    click::web::CallParams params;
    params.add(click::QUERY_ARGNAME, FAKE_QUERY);
    EXPECT_CALL(*servicePtr, callImpl(_, _, _, _, _, params))
            .Times(1)
            .WillOnce(Return(response));

    indexPtr->search(FAKE_QUERY, [](click::PackageList) {});
}

TEST_F(IndexTest, testSearchSendsRightPath)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(*servicePtr, callImpl(EndsWith(click::SEARCH_PATH),
                                      _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));

    indexPtr->search("", [](click::PackageList) {});
}

TEST_F(IndexTest, testSearchCallbackIsCalled)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    QByteArray fake_json("[]");
    EXPECT_CALL(reply.instance, readAll())
            .Times(1)
            .WillOnce(Return(fake_json));
    EXPECT_CALL(*servicePtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));
    EXPECT_CALL(*this, search_callback(_)).Times(1);

    indexPtr->search("", [this](click::PackageList packages){
        search_callback(packages);
    });
    response->replyFinished();
}

TEST_F(IndexTest, testSearchEmptyJsonIsParsed)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    QByteArray fake_json("[]");
    EXPECT_CALL(reply.instance, readAll())
            .Times(1)
            .WillOnce(Return(fake_json));
    EXPECT_CALL(*servicePtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));
    click::PackageList empty_package_list;
    EXPECT_CALL(*this, search_callback(empty_package_list)).Times(1);

    indexPtr->search("", [this](click::PackageList packages){
        search_callback(packages);
    });
    response->replyFinished();
}

TEST_F(IndexTest, testSearchSingleJsonIsParsed)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    QByteArray fake_json(FAKE_JSON_SEARCH_RESULT_ONE.c_str());
    EXPECT_CALL(reply.instance, readAll())
            .Times(1)
            .WillOnce(Return(fake_json));
    EXPECT_CALL(*servicePtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));
    click::PackageList single_package_list =
    {
        click::Package
        {
            "org.example.awesomelauncher", "Awesome Launcher", "1.99",
            "http://software-center.ubuntu.com/site_media/appmedia/2012/09/SPAZ.png",
            "http://search.apps.ubuntu.com/api/v1/package/org.example.awesomelauncher"
        }
    };
    EXPECT_CALL(*this, search_callback(single_package_list)).Times(1);

    indexPtr->search("", [this](click::PackageList packages){
        search_callback(packages);
    });
    response->replyFinished();
}

TEST_F(IndexTest, DISABLED_testInvalidJsonIsIgnored)
{
    // TODO, in upcoming branch
}

TEST_F(IndexTest, DISABLED_testNetworkErrorIgnored)
{
    // TODO, in upcoming branch
}

TEST_F(IndexTest, testGetDetailsCallsWebservice)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(*servicePtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));

    indexPtr->get_details("", [](click::PackageDetails) {});
}

TEST_F(IndexTest, testGetDetailsSendsPackagename)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(*servicePtr, callImpl(EndsWith(FAKE_PACKAGENAME),
                                      _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));

    indexPtr->get_details(FAKE_PACKAGENAME, [](click::PackageDetails) {});
}

TEST_F(IndexTest, testGetDetailsSendsRightPath)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(*servicePtr, callImpl(StartsWith(click::SEARCH_BASE_URL +
                                                 click::DETAILS_PATH),
                                      _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));

    indexPtr->get_details(FAKE_PACKAGENAME, [](click::PackageDetails) {});
}

TEST_F(IndexTest, testGetDetailsCallbackIsCalled)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    QByteArray fake_json(FAKE_JSON_PACKAGE_DETAILS.c_str());
    EXPECT_CALL(reply.instance, readAll())
            .Times(1)
            .WillOnce(Return(fake_json));
    EXPECT_CALL(*servicePtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));
    indexPtr->get_details("", [this](click::PackageDetails details){
        details_callback(details);
    });
    EXPECT_CALL(*this, details_callback(_)).Times(1);
    response->replyFinished();
}

TEST_F(IndexTest, testGetDetailsJsonIsParsed)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    QByteArray fake_json(FAKE_JSON_PACKAGE_DETAILS.c_str());
    EXPECT_CALL(reply.instance, readAll())
            .Times(1)
            .WillOnce(Return(fake_json));
    EXPECT_CALL(*servicePtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));
    indexPtr->get_details("", [this](click::PackageDetails details){
        details_callback(details);
    });
    click::PackageDetails fake_details {
        "ar.com.beuno.wheather-touch",
        "Weather",
        "http://developer.staging.ubuntu.com/site_media/appmedia/2013/07/weather-icone-6797-64.png",
        "Weather\nA weather application.",
        "https://public.apps.staging.ubuntu.com/download/ar.com.beuno/wheather-touch/ar.com.beuno.wheather-touch-0.2",
        "3.5",
        "",
        "",
        "Proprietary",
        "Beuno",
        "",
        {"sshot1", "sshot2"},
        "177582",
        "0.2",
        "None"
    };
    EXPECT_CALL(*this, details_callback(fake_details)).Times(1);
    response->replyFinished();
}
