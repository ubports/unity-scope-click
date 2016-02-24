/*
 * Copyright (C) 2014-2016 Canonical Ltd.
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

#include <click/download-manager.h>
#include <mock_ubuntu_download_manager.h>
#include <tests/mock_network_access_manager.h>
#include <tests/mock_webclient.h>

#include <gtest/gtest.h>
#include <memory>

using namespace ::testing;

namespace udm = Ubuntu::DownloadManager;
#include <ubuntu/download_manager/download_struct.h>

namespace
{
const QString TEST_URL("http://test.local/");
const QString TEST_SHA512("fake_hash");
const QString TEST_HEADER_VALUE("test header value");
const QString TEST_APP_ID("test_app_id");
const QString TEST_CLICK_TOKEN_VALUE("test token value");
const QString TEST_DOWNLOAD_ID("/com/ubuntu/download_manager/test");
const QString TEST_DOWNLOADERROR_STRING("test downloadError string");


class DownloadManagerTest : public ::testing::Test
{
protected:
    QSharedPointer<MockClient> clientPtr;
    QSharedPointer<MockNetworkAccessManager> namPtr;
    QSharedPointer<MockSystemDownloadManager> sdmPtr;
    std::shared_ptr<click::DownloadManager> dmPtr;

    virtual void SetUp()
    {
        namPtr.reset(new MockNetworkAccessManager());
        clientPtr.reset(new NiceMock<MockClient>(namPtr));
        dmPtr.reset(new click::DownloadManager(clientPtr, sdmPtr));
    }

    MOCK_METHOD2(start_callback, void(std::string, click::DownloadManager::Error));
    MOCK_METHOD1(progress_callback, void(std::string));
};

}

TEST_F(DownloadManagerTest, testStartCallsWebservice)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));

    dmPtr->start("", "", "",
                 [](std::string, click::DownloadManager::Error) {});
}

TEST_F(DownloadManagerTest, testStartCallbackCalled)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(reply.instance, attribute(_)).WillOnce(Return(QVariant(0)));
    EXPECT_CALL(reply.instance, readAll())
            .Times(1)
            .WillOnce(Return(""));
    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));
    EXPECT_CALL(*this, start_callback(_, _)).Times(1);

    dmPtr->start("", "", "",
                 [this](std::string msg, click::DownloadManager::Error err) {
                     start_callback(msg, err);
                 });
    response->replyFinished();
}

TEST_F(DownloadManagerTest, testStartHTTPForbidden)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(reply.instance, attribute(_)).WillOnce(Return(QVariant(403)));
    EXPECT_CALL(reply.instance, readAll())
            .Times(1)
            .WillOnce(Return(""));
    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));
    EXPECT_CALL(*this, start_callback(StartsWith("Unhandled HTTP response code:"),
                                      click::DownloadManager::Error::DownloadInstallError)).Times(1);

    dmPtr->start("", "", "",
                 [this](std::string msg, click::DownloadManager::Error err) {
                     start_callback(msg, err);
                 });
    response->replyFinished();
}

TEST_F(DownloadManagerTest, testStartHTTPError)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(reply.instance, errorString())
        .WillOnce(Return(QString("ERROR")));
    EXPECT_CALL(reply.instance, attribute(_)).WillOnce(Return(QVariant(404)));
    EXPECT_CALL(reply.instance, readAll())
            .Times(1)
            .WillOnce(Return(""));
    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));
    EXPECT_CALL(*this, start_callback("ERROR (203)",
                                      click::DownloadManager::Error::DownloadInstallError)).Times(1);

    dmPtr->start("", "", "",
                 [this](std::string msg, click::DownloadManager::Error err) {
                     start_callback(msg, err);
                 });
    response->errorHandler(QNetworkReply::ContentNotFoundError);
}

TEST_F(DownloadManagerTest, testStartCredentialsError)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(reply.instance, errorString())
        .WillOnce(Return(QString("ERROR")));
    EXPECT_CALL(reply.instance, attribute(_)).WillOnce(Return(QVariant(401)));
    EXPECT_CALL(reply.instance, readAll())
            .Times(1)
            .WillOnce(Return(""));
    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));
    EXPECT_CALL(*this, start_callback("ERROR (201)",
                                      click::DownloadManager::Error::CredentialsError)).Times(1);

    dmPtr->start("", "", "test.package",
                 [this](std::string msg, click::DownloadManager::Error err) {
                     start_callback(msg, err);
                 });
    response->errorHandler(QNetworkReply::ContentAccessDenied);
}

// FIXME: Needs Qt main loop
TEST_F(DownloadManagerTest, DISABLED_testGetProgressNoDownloads)
{
    EXPECT_CALL(*sdmPtr, getAllDownloadsWithMetadata(_, _, _, _))
        .Times(1)
        .WillOnce(InvokeArgument<3>(QStringLiteral(""), QStringLiteral(""),
                                    nullptr));
    dmPtr->get_progress("com.example.test",
                        [this](std::string object_path) {
                            progress_callback(object_path);
                        });
}
