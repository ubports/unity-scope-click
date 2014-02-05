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
#include <QString>
#include <QThread>
#include <QTimer>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <click/download-manager.h>

#include "mock_network_access_manager.h"
#include "mock_ubuntuone_credentials.h"

namespace
{
const QString TEST_URL("http://test.local/");

struct DownloadManagerTest : public ::testing::Test
{
    DownloadManagerTest()
        : app(argc, argv),
          mockNam(new MockNetworkAccessManager()),
          mockCredentialsService(new MockCredentialsService()),
          mockReplyPtr(&mockReply, [](click::network::Reply*) {})
    {
        signalTimer.setSingleShot(true);
        testTimeout.setSingleShot(true);

        QObject::connect(
                    &testTimeout, &QTimer::timeout,
                    [this]() { app.quit(); FAIL() << "Operation timed out."; } );
    }

    void signalEmptyTokenFromMockCredsService()
    {
        click::Token token;
        mockCredentialsService->credentialsFound(token);
    }

    void signalErrorAfterDelay()
    {
        // delay emitting this signal so that the download manager has
        // time to connect to the signal first, as the (mock)Reply is
        // returned by the (mock)Nam.
        QObject::connect(&signalTimer, &QTimer::timeout, [this]()
        {
            mockReplyPtr->error(QNetworkReply::UnknownNetworkError);
        });
        signalTimer.start(10);
    }

    void SetUp()
    {
        const int oneSecondInMsec = 1000;
        testTimeout.start(10 * oneSecondInMsec);
    }

    void Quit()
    {
        app.quit();
    }

    int argc = 0;
    char** argv = nullptr;
    QCoreApplication app;
    QTimer testTimeout;
    QTimer signalTimer;
    QSharedPointer<MockNetworkAccessManager> mockNam;
    QSharedPointer<MockCredentialsService> mockCredentialsService;
    MockNetworkReply mockReply;
    QSharedPointer<click::network::Reply> mockReplyPtr;
};


struct DownloadManagerMockClient {
    MOCK_METHOD1(onFetchClickErrorEmitted, void(QString errorMessage));
};

} // anon namespace


TEST_F(DownloadManagerTest, testFetchClickTokenCredentialsNotFound)
{
    using namespace ::testing;

    // Mock the credentials service, to signal that creds are not found:
    EXPECT_CALL(*mockCredentialsService, getCredentials())
        .Times(1).WillOnce(
                InvokeWithoutArgs(mockCredentialsService.data(),
                              &MockCredentialsService::credentialsNotFound));

    // We should not hit the NAM without creds:
    EXPECT_CALL(*mockNam, head(_)).Times(0);

    // Objects under test should be instantiated in the respective test case.
    click::DownloadManager dm(mockNam, mockCredentialsService);

    // Connect the mock client to downloadManager's error signal,
    // to check that it sends appropriate signals:

    DownloadManagerMockClient mockDownloadManagerClient;
    QObject::connect(&dm, &click::DownloadManager::clickTokenFetchError,
                     [&mockDownloadManagerClient](const QString& error)
                     {
                         mockDownloadManagerClient.onFetchClickErrorEmitted(error);
                     });

    // The error signal should be sent once, and we clean up the
    // test's QCoreApplication in its handler:
    EXPECT_CALL(mockDownloadManagerClient, onFetchClickErrorEmitted(_))
            .Times(1)
            .WillOnce(
                InvokeWithoutArgs(
                    this,
                    &DownloadManagerTest::Quit));

    // Now start the function we're testing, after a delay. This is
    // awkwardly verbose because QTimer::singleShot doesn't accept
    // arguments or lambdas.

    // We need to delay the call until after the app.exec() call so
    // that when we call app.quit() on success, there is a running app
    // to quit.
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, [&dm]() {
            dm.fetchClickToken(TEST_URL);
        } );
    timer.start(0);
    
    // now exec the app so events can proceed:
    app.exec();
}


// Test Cases:

TEST_F(DownloadManagerTest, testFetchClickTokenCredsFoundButNetworkError)
{
    using namespace ::testing;

    // Mock the credentials service, to signal that creds are found.

    EXPECT_CALL(*mockCredentialsService, getCredentials())
        .Times(1).WillOnce(
            InvokeWithoutArgs(this, &DownloadManagerTest::signalEmptyTokenFromMockCredsService));

    EXPECT_CALL(mockReply, errorString()).Times(1).WillOnce(Return(QString("Bogus error for tests")));

    EXPECT_CALL(*mockNam, head(_)).WillOnce(
                DoAll(
                    InvokeWithoutArgs(this, &DownloadManagerTest::signalErrorAfterDelay),
                    Return(mockReplyPtr)));

    // Objects under test should be instantiated in the respective test case.
    click::DownloadManager dm(mockNam, mockCredentialsService);

    // Connect the mock client to downloadManager's error signal,
    // to check that it sends appropriate signals:

    DownloadManagerMockClient mockDownloadManagerClient;
    QObject::connect(&dm, &click::DownloadManager::clickTokenFetchError,
                     [&mockDownloadManagerClient](const QString& error)
                     {
                         mockDownloadManagerClient.onFetchClickErrorEmitted(error);
                     });

    // The error signal should be sent once, and we clean up the
    // test's QCoreApplication in its handler:
    EXPECT_CALL(mockDownloadManagerClient, onFetchClickErrorEmitted(_))
            .Times(1)
            .WillOnce(
                InvokeWithoutArgs(
                    this,
                    &DownloadManagerTest::Quit));

    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, [&dm]() {
            dm.fetchClickToken(TEST_URL);
        } );
    timer.start(0);

    app.exec();
}

// void TestDownloadManager::testFetchClickTokenSuccess()
// {
//     _tdm.setShouldSignalCredsFound(true);
//     FakeNam::shouldSignalNetworkError = false;
//     QSignalSpy spy(&_tdm, SIGNAL(clickTokenFetched(QString)));
//     _tdm.fetchClickToken(TEST_URL);
//     QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, SCOPE_TEST_TIMEOUT_MSEC);
// }
