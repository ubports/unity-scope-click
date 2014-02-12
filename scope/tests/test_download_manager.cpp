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

#include <QDBusObjectPath>
#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include <QThread>
#include <QTimer>

#include <token.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <click/download-manager.h>

#include "mock_network_access_manager.h"
#include "mock_ubuntuone_credentials.h"
#include "mock_ubuntu_download_manager.h"

namespace udm = Ubuntu::DownloadManager;
#include <ubuntu/download_manager/download_struct.h>


namespace
{
const QString TEST_URL("http://test.local/");
const QString TEST_HEADER_VALUE("test header value");
const QString TEST_APP_ID("test_app_id");
const QString TEST_CLICK_TOKEN_VALUE("test token value");
const QString TEST_DOWNLOAD_ID("/com/ubuntu/download_manager/test");
const QString TEST_DOWNLOADERROR_STRING("test downloadError string");

struct CredsNetworkTestParameters
{
public:
    CredsNetworkTestParameters(bool credsFound = true, bool replySignalsError = false,
                   int replyStatusCode = 200, bool replyHasClickRawHeader = true,
                   bool expectSuccessSignal = true)
        : credsFound(credsFound), replySignalsError(replySignalsError), replyStatusCode(replyStatusCode),
          replyHasClickRawHeader(replyHasClickRawHeader), expectSuccessSignal(expectSuccessSignal) {};

    bool credsFound;
    bool replySignalsError;
    int replyStatusCode;
    bool replyHasClickRawHeader;
    bool expectSuccessSignal;
};

::std::ostream& operator<<(::std::ostream& os, const CredsNetworkTestParameters& p)
{
    return os << "creds[" << (p.credsFound ? "x" : " ") << "] "
              << "replySignalsError[" << (p.replySignalsError ? "x" : " ") << "] "
              << "statusCode[" << p.replyStatusCode << "] "
              << "replyHasClickRawHeader[" << (p.replyHasClickRawHeader ? "x" : " ") << "] "
              << "expectSuccessSignal[" << (p.expectSuccessSignal ? "x" : " ") << "] ";
}


struct StartDownloadTestParameters
{
public:
    StartDownloadTestParameters(bool clickTokenFetchSignalsError = false,
                                bool downloadSignalsError = false,
                                bool expectSuccessSignal = true)
        : clickTokenFetchSignalsError(clickTokenFetchSignalsError),
          downloadSignalsError(downloadSignalsError), 
          expectSuccessSignal(expectSuccessSignal) {};

    bool clickTokenFetchSignalsError;
    bool downloadSignalsError;
    bool expectSuccessSignal;
};

::std::ostream& operator<<(::std::ostream& os, const StartDownloadTestParameters& p)
{
    return os << "clickTokenFetchSignalsError[" << (p.clickTokenFetchSignalsError ? "x" : " ") << "] "
              << "downloadSignalsError[" << (p.downloadSignalsError ? "x" : " ") << "] "
              << "expectSuccessSignal[" << (p.expectSuccessSignal ? "x" : " ") << "] ";
}


struct DownloadManagerTestBase
{
    DownloadManagerTestBase()
        : app(argc, argv),
          mockNam(new MockNetworkAccessManager()),
          mockCredentialsService(new MockCredentialsService()),
          mockReplyPtr(&mockReply, [](click::network::Reply*) {}),
          mockSystemDownloadManager(new MockSystemDownloadManager())
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
    ::testing::NiceMock<MockNetworkReply> mockReply;
    QSharedPointer<click::network::Reply> mockReplyPtr;
    QSharedPointer<MockSystemDownloadManager> mockSystemDownloadManager;
};

struct DISABLED_DownloadManagerStartDownloadTest : public DownloadManagerTestBase,
                                          public ::testing::TestWithParam<StartDownloadTestParameters>
{
public:
};

struct DISABLED_DownloadManagerCredsNetworkTest : public DownloadManagerTestBase,
                                         public ::testing::TestWithParam<CredsNetworkTestParameters>
{
public:

    void signalEmptyTokenFromMockCredsService()
    {
        UbuntuOne::Token token;
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

    void signalFinishedAfterDelay()
    {
        QObject::connect(&signalTimer, &QTimer::timeout, [this]()
        {
            mockReplyPtr->finished();
        });
        signalTimer.start(10);
    }
    
};

struct DownloadManagerMockClient
{
    MOCK_METHOD1(onClickTokenFetchedEmitted, void(QString clickToken));
    MOCK_METHOD1(onClickTokenFetchErrorEmitted, void(QString errorMessage));
    MOCK_METHOD1(onDownloadStartedEmitted, void(QString id));
    MOCK_METHOD1(onDownloadErrorEmitted, void(QString errorMessage));
};

} // anon namespace


TEST_P(DISABLED_DownloadManagerCredsNetworkTest, TestFetchClickToken)
{
    using namespace ::testing;

    CredsNetworkTestParameters p = GetParam();

    QList<QPair<QByteArray, QByteArray> > emptyHeaderPairs;
    ON_CALL(mockReply, rawHeaderPairs()).WillByDefault(Return(emptyHeaderPairs));
    ON_CALL(mockReply, readAll()).WillByDefault(Return(QByteArray("bogus readAll() return")));

    if (p.credsFound) {

        EXPECT_CALL(*mockCredentialsService, getCredentials())
            .Times(1).WillOnce(
                InvokeWithoutArgs(this,
                                  &DISABLED_DownloadManagerCredsNetworkTest::signalEmptyTokenFromMockCredsService));

        if (p.replySignalsError) {
            EXPECT_CALL(*mockNam, head(_)).WillOnce(
                DoAll(
                    InvokeWithoutArgs(this, &DISABLED_DownloadManagerCredsNetworkTest::signalErrorAfterDelay),
                    Return(mockReplyPtr)));
            EXPECT_CALL(mockReply, errorString()).Times(1).WillOnce(Return(QString("Bogus error for tests")));

        } else {
            EXPECT_CALL(*mockNam, head(_)).WillOnce(
                DoAll(
                    InvokeWithoutArgs(this, &DISABLED_DownloadManagerCredsNetworkTest::signalFinishedAfterDelay),
                    Return(mockReplyPtr)));

            EXPECT_CALL(mockReply, attribute(QNetworkRequest::HttpStatusCodeAttribute))
                .Times(1).WillOnce(Return(QVariant(p.replyStatusCode)));

            if (p.replyStatusCode == 200) {
                EXPECT_CALL(mockReply, hasRawHeader(click::CLICK_TOKEN_HEADER()))
                    .Times(1).WillOnce(Return(p.replyHasClickRawHeader));

                if (p.replyHasClickRawHeader) {
                    EXPECT_CALL(mockReply, rawHeader(click::CLICK_TOKEN_HEADER()))
                        .Times(1).WillOnce(Return(TEST_HEADER_VALUE));
                }
            }

        }

    } else {
        EXPECT_CALL(*mockCredentialsService, getCredentials())
            .Times(1).WillOnce(InvokeWithoutArgs(mockCredentialsService.data(),
                                                 &MockCredentialsService::credentialsNotFound));

        EXPECT_CALL(*mockNam, head(_)).Times(0);
    }

    click::DownloadManager dm(mockNam, mockCredentialsService,
                              mockSystemDownloadManager);

    DownloadManagerMockClient mockDownloadManagerClient;

    QObject::connect(&dm, &click::DownloadManager::clickTokenFetchError,
                     [&mockDownloadManagerClient](const QString& error)
                     {
                         mockDownloadManagerClient.onClickTokenFetchErrorEmitted(error);
                     });


    QObject::connect(&dm, &click::DownloadManager::clickTokenFetched,
                     [&mockDownloadManagerClient](const QString& token)
                     {
                         mockDownloadManagerClient.onClickTokenFetchedEmitted(token);
                     });

    if (p.expectSuccessSignal) {

        EXPECT_CALL(mockDownloadManagerClient, onClickTokenFetchedEmitted(TEST_HEADER_VALUE))
            .Times(1)
            .WillOnce(
                InvokeWithoutArgs(
                    this,
                    &DISABLED_DownloadManagerCredsNetworkTest::Quit));

        EXPECT_CALL(mockDownloadManagerClient, onClickTokenFetchErrorEmitted(_)).Times(0);

    } else {

        EXPECT_CALL(mockDownloadManagerClient, onClickTokenFetchErrorEmitted(_))
            .Times(1)
            .WillOnce(
                InvokeWithoutArgs(
                    this,
                    &DISABLED_DownloadManagerCredsNetworkTest::Quit));

        EXPECT_CALL(mockDownloadManagerClient, onClickTokenFetchedEmitted(_)).Times(0);

    }

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

INSTANTIATE_TEST_CASE_P(DownloadManagerCredsNetworkTests, DISABLED_DownloadManagerCredsNetworkTest,
                        ::testing::Values(
                            // CredsNetworkTestParameters(credsFound, replySignalsError, replyStatusCode, replyHasClickRawHeader, expectSuccessSignal)
                            CredsNetworkTestParameters(true, false, 200, true, true), // success
                            CredsNetworkTestParameters(true, true, 200, true, false), // misc QNetworkReply error => error
                            CredsNetworkTestParameters(true, false, 200, false, false), // no header => error
                            CredsNetworkTestParameters(true, false, 401, true, false), // HTTP error status => error
                            CredsNetworkTestParameters(false, false, 200, true, false) // no creds => error
                            ));


MATCHER(DownloadStructIsValid, "Struct is not valid")
{
    return arg.getUrl() == TEST_URL
        && arg.getHash() == "" 
        && arg.getAlgorithm() == ""
        && arg.getMetadata()["app_id"] == QVariant(TEST_APP_ID)
        && arg.getMetadata()["post-download-command"].toStringList().length() == 4
        && arg.getHeaders()["X-Click-Token"] == TEST_CLICK_TOKEN_VALUE;
}


TEST_P(DISABLED_DownloadManagerStartDownloadTest, TestStartDownload)
{
    using namespace ::testing;

    StartDownloadTestParameters p = GetParam();

    click::DownloadManager dm(mockNam, mockCredentialsService,
                              mockSystemDownloadManager);

    // mockError is heap-allocated because downloadWithError will delete it.
    MockError *mockError = new MockError();
    MockDownload downloadWithError(mockError);
    MockDownload successfulDownload;

    // Just directly signal clickTokenFetched or error from
    // getCredentials(), no need to re-test the same code as the
    // previous test

    std::function<void()> clickTokenSignalFunc;
    if (p.clickTokenFetchSignalsError) {
        clickTokenSignalFunc = std::function<void()>([&](){ 
                dm.clickTokenFetchError(TEST_DOWNLOADERROR_STRING);
            });
        EXPECT_CALL(*mockSystemDownloadManager, createDownload(_)).Times(0);

    } else {
        clickTokenSignalFunc = std::function<void()>([&](){ 
                dm.clickTokenFetched(TEST_CLICK_TOKEN_VALUE);
            });

        std::function<void()> downloadCreatedSignalFunc;

        // NOTE: udm::Download doesn't have virtual functions, so mocking
        // them doesn't work and we have to construct objects that will
        // behave correctly without mock return values, using overridden constructors:
        if (p.downloadSignalsError) {

            EXPECT_CALL(*mockError, errorString()).Times(1).WillOnce(Return(TEST_DOWNLOADERROR_STRING));
            downloadCreatedSignalFunc = std::function<void()>([&](){
                    mockSystemDownloadManager->downloadCreated(&downloadWithError);
                });

        } else {
            EXPECT_CALL(*mockError, errorString()).Times(0);
            downloadCreatedSignalFunc = std::function<void()>([&](){
                    mockSystemDownloadManager->downloadCreated(&successfulDownload);
                });
        }

        EXPECT_CALL(*mockSystemDownloadManager, 
                    createDownload(DownloadStructIsValid())).Times(1).WillOnce(InvokeWithoutArgs(downloadCreatedSignalFunc));
    }        

    EXPECT_CALL(*mockCredentialsService, getCredentials())
        .Times(1).WillOnce(InvokeWithoutArgs(clickTokenSignalFunc));

    
    DownloadManagerMockClient mockDownloadManagerClient;

    QObject::connect(&dm, &click::DownloadManager::downloadError,
                     [&mockDownloadManagerClient](const QString& error)
                     {
                         mockDownloadManagerClient.onDownloadErrorEmitted(error);
                     });


    QObject::connect(&dm, &click::DownloadManager::downloadStarted,
                     [&mockDownloadManagerClient](const QString& downloadId)
                     {
                         mockDownloadManagerClient.onDownloadStartedEmitted(downloadId);
                     });

    if (p.expectSuccessSignal) {

        EXPECT_CALL(mockDownloadManagerClient, onDownloadStartedEmitted(TEST_DOWNLOAD_ID))
            .Times(1)
            .WillOnce(
                InvokeWithoutArgs(
                    this,
                    &DISABLED_DownloadManagerStartDownloadTest::Quit));

        EXPECT_CALL(mockDownloadManagerClient, onDownloadErrorEmitted(_)).Times(0);
  
        // when https://bugs.launchpad.net/ubuntu-download-manager/+bug/1278789
        // is fixed, we can add this assertion:
        // EXPECT_CALL(successfulDownload, start()).Times(1);

    } else {

        EXPECT_CALL(mockDownloadManagerClient, onDownloadErrorEmitted(TEST_DOWNLOADERROR_STRING))
            .Times(1)
            .WillOnce(
                InvokeWithoutArgs(
                    this,
                    &DISABLED_DownloadManagerStartDownloadTest::Quit));

        EXPECT_CALL(mockDownloadManagerClient, onDownloadStartedEmitted(_)).Times(0);

    }
    
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, [&dm]() {
            dm.startDownload(TEST_URL, TEST_APP_ID);
        } );
    timer.start(0);

    // now exec the app so events can proceed:
    app.exec();

}

INSTANTIATE_TEST_CASE_P(DownloadManagerStartDownloadTests, DISABLED_DownloadManagerStartDownloadTest,
                        ::testing::Values(
                            // params: (clickTokenFetchSignalsError, downloadSignalsError, expectSuccessSignal)
                            StartDownloadTestParameters(false, false, true),
                            StartDownloadTestParameters(true, false, false),
                            StartDownloadTestParameters(false, true, false)
                            ));
