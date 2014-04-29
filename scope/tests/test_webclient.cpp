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
#include <QDebug>

#include "click/webclient.h"

#include "mock_network_access_manager.h"
#include "mock_ubuntuone_credentials.h"

#include <gtest/gtest.h>

const std::string FAKE_SERVER = "http://fake-server/";
const std::string FAKE_PATH = "fake/api/path";

MATCHER_P(IsCorrectUrl, refUrl, "")
{
    *result_listener << "where the url is " << qPrintable(arg.url().toString());
    return arg.url().toString() == refUrl;
}

MATCHER_P(IsValidOAuthHeader, refOAuth, "")
{
    return arg.hasRawHeader(click::web::AUTHORIZATION_HEADER.c_str()) && arg.rawHeader(click::web::AUTHORIZATION_HEADER.c_str())
        .startsWith("OAuth ");
}

MATCHER_P(IsCorrectCookieHeader, refCookie, "")
{
    return arg.hasRawHeader("Cookie") && arg.rawHeader("Cookie") == refCookie;
}

MATCHER_P(IsCorrectBufferData, refData, "")
{
    return dynamic_cast<QBuffer*>(arg)->data() == refData;
}

class WebClientTest : public ::testing::Test
{
public:
    MockNetworkAccessManager nam;
    QSharedPointer<click::network::AccessManager> namPtr;
    MockCredentialsService sso;
    QSharedPointer<click::CredentialsService> ssoPtr;

    void SetUp()
    {
        namPtr.reset(&nam, [](click::network::AccessManager*) {});
        ssoPtr.reset(&sso, [](click::CredentialsService*) {});
    }

    MOCK_METHOD0(replyHandler, void());
    MOCK_METHOD1(errorHandler, void(QString description));
};

TEST_F(WebClientTest, testUrlBuiltNoParams)
{
    using namespace ::testing;

    auto reply = new NiceMock<MockNetworkReply>();
    ON_CALL(*reply, readAll()).WillByDefault(Return("HOLA"));
    QSharedPointer<click::network::Reply> replyPtr(reply);

    click::web::Client wc(namPtr);

    EXPECT_CALL(nam, sendCustomRequest(IsCorrectUrl(QString("http://fake-server/fake/api/path")), _, _))
            .Times(1)
            .WillOnce(Return(replyPtr));

    auto wr = wc.call(FAKE_SERVER + FAKE_PATH);
}

TEST_F(WebClientTest, testParamsAppended)
{
    using namespace ::testing;

    auto reply = new NiceMock<MockNetworkReply>();
    ON_CALL(*reply, readAll()).WillByDefault(Return("HOLA"));
    QSharedPointer<click::network::Reply> replyPtr(reply);

    click::web::Client wc(namPtr);

    click::web::CallParams params;
    params.add("a", "1");
    params.add("b", "2");

    EXPECT_CALL(nam, sendCustomRequest(IsCorrectUrl(QString("http://fake-server/fake/api/path?a=1&b=2")), _, _))
            .Times(1)
            .WillOnce(Return(replyPtr));

    auto wr = wc.call(FAKE_SERVER + FAKE_PATH, params);
}

/*
TEST(WebClient, testResultsAreEmmited)
{
    FakeNam::scripted_responses.append("HOLA");
    click::web::Client wc(
                FAKE_SERVER,
                QSharedPointer<click::network::AccessManager>(new click::network::AccessManager()));

    auto wr = wc.call(FAKE_SERVER + FAKE_PATH);
    connect(wr.data(), &click::web::Response::finished, this, &TestWebClient::gotResults);
    QTRY_COMPARE(results, QString("HOLA"));

    using namespace ::testing;

    MockNetworkAccessManager nam;
    QSharedPointer<click::network::AccessManager> namPtr(
                &nam,
                [](click::network::AccessManager*) {});

    auto reply = new NiceMock<MockNetworkReply>();
    ON_CALL(*reply, readAll()).WillByDefault(Return("HOLA"));

    click::web::Client wc(namPtr);
    auto wr = wc.call(FAKE_SERVER + FAKE_PATH);

    // TODO: We need to extend the web::Response class to allow for reading the contents of the response
    // EXPECT_EQ(QByteArray("HOLA"), wr->);
}
*/

TEST_F(WebClientTest, testCookieHeaderSetCorrectly)
{
    using namespace ::testing;

    auto reply = new NiceMock<MockNetworkReply>();
    ON_CALL(*reply, readAll()).WillByDefault(Return("HOLA"));
    QSharedPointer<click::network::Reply> replyPtr(reply);

    click::web::Client wc(namPtr);

    EXPECT_CALL(nam, sendCustomRequest(IsCorrectCookieHeader("CookieCookieCookie"), _, _))
            .Times(1)
            .WillOnce(Return(replyPtr));

    auto wr = wc.call(FAKE_SERVER + FAKE_PATH,
                      "GET", false,
                      std::map<std::string, std::string>({{"Cookie", "CookieCookieCookie"}}));
}

TEST_F(WebClientTest, testMethodPassedCorrectly)
{
    using namespace ::testing;

    auto reply = new NiceMock<MockNetworkReply>();
    ON_CALL(*reply, readAll()).WillByDefault(Return("HOLA"));
    QSharedPointer<click::network::Reply> replyPtr(reply);

    click::web::Client wc(namPtr);

    QByteArray verb("POST", 4);
    EXPECT_CALL(nam, sendCustomRequest(_, verb, _))
            .Times(1)
            .WillOnce(Return(replyPtr));

    auto wr = wc.call(FAKE_SERVER + FAKE_PATH,
                      "POST", false);
}

TEST_F(WebClientTest, testBufferDataPassedCorrectly)
{
    using namespace ::testing;

    auto reply = new NiceMock<MockNetworkReply>();
    ON_CALL(*reply, readAll()).WillByDefault(Return("HOLA \u5c0f\u6d77"));
    QSharedPointer<click::network::Reply> replyPtr(reply);

    click::web::Client wc(namPtr);

    EXPECT_CALL(nam, sendCustomRequest(_, _, IsCorrectBufferData("HOLA \u5c0f\u6d77")))
            .Times(1)
            .WillOnce(Return(replyPtr));

    auto wr = wc.call(FAKE_SERVER + FAKE_PATH,
                      "POST", false, std::map<std::string, std::string>(),
                      "HOLA \u5c0f\u6d77");
}

TEST_F(WebClientTest, testSignedCorrectly)
{
    using namespace ::testing;

    auto reply = new NiceMock<MockNetworkReply>();
    ON_CALL(*reply, readAll()).WillByDefault(Return("HOLA"));
    QSharedPointer<click::network::Reply> replyPtr(reply);

    click::web::Client wc(namPtr);
    wc.setCredentialsService(ssoPtr);

    EXPECT_CALL(sso, getCredentials()).WillOnce(Invoke([&](){
                UbuntuOne::Token token("token_key", "token_secret",
                                       "consumer_key", "consumer_secret");
                sso.credentialsFound(token);
            }));
    EXPECT_CALL(nam, sendCustomRequest(IsValidOAuthHeader(""), _, _))
            .Times(1)
            .WillOnce(Return(replyPtr));

    auto wr = wc.call(FAKE_SERVER + FAKE_PATH,
                      "HEAD", true);
}

TEST_F(WebClientTest, testSignTokenNotFound)
{
    using namespace ::testing;

    auto reply = new NiceMock<MockNetworkReply>();
    ON_CALL(*reply, readAll()).WillByDefault(Return("HOLA"));
    QSharedPointer<click::network::Reply> replyPtr(reply);

    click::web::Client wc(namPtr);
    wc.setCredentialsService(ssoPtr);

    EXPECT_CALL(sso, getCredentials()).WillOnce(Invoke([&]() {
                sso.credentialsNotFound();
            }));
    EXPECT_CALL(nam, sendCustomRequest(_, _, _)).Times(0);

    auto wr = wc.call(FAKE_SERVER + FAKE_PATH,
                      "HEAD", true);
}


TEST_F(WebClientTest, testResponseFinished)
{
    using namespace ::testing;

    auto reply = new NiceMock<MockNetworkReply>();
    ON_CALL(*reply, readAll()).WillByDefault(Return("HOLA"));
    QSharedPointer<click::network::Reply> replyPtr(reply);

    click::web::Client wc(namPtr);

    EXPECT_CALL(nam, sendCustomRequest(_, _, _))
            .Times(1)
            .WillOnce(Return(replyPtr));

    auto response = wc.call(FAKE_SERVER + FAKE_PATH);
    QObject::connect(response.data(), &click::web::Response::finished, [this](){replyHandler();});
    EXPECT_CALL(*this, replyHandler());
    emit reply->finished();
}

TEST_F(WebClientTest, testResponseFailed)
{
    using namespace ::testing;

    auto reply = new NiceMock<MockNetworkReply>();
    QSharedPointer<click::network::Reply> replyPtr(reply);

    click::web::Client wc(namPtr);

    EXPECT_CALL(nam, sendCustomRequest(_, _, _))
            .Times(1)
            .WillOnce(Return(replyPtr));
    EXPECT_CALL(*reply, errorString()).Times(1).WillOnce(Return("fake error"));

    auto response = wc.call(FAKE_SERVER + FAKE_PATH);
    QObject::connect(response.data(), &click::web::Response::error,
                     [this, &response](QString desc){
                         errorHandler(desc);
                         Q_UNUSED(response);
                     });

    EXPECT_CALL(*this, errorHandler(_));
    emit reply->error(QNetworkReply::UnknownNetworkError);
}

TEST_F(WebClientTest, testResponseAbort)
{
    using namespace ::testing;

    auto reply = new NiceMock<MockNetworkReply>();
    QSharedPointer<click::network::Reply> replyPtr(reply);

    click::web::Client wc(namPtr);

    EXPECT_CALL(nam, sendCustomRequest(_, _, _))
            .Times(1)
            .WillOnce(Return(replyPtr));

    auto response = wc.call(FAKE_SERVER + FAKE_PATH);

    EXPECT_CALL(*reply, abort()).Times(1);
    response->abort();
}
