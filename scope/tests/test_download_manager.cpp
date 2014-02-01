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

#include "test_download_manager.h"

#define SCOPE_TEST_TIMEOUT_MSEC 5000

static const QString TEST_URL("http://test.local/");

void TestableDownloadManager::setShouldSignalCredsFound(bool shouldSignalCredsFound)
{
    _shouldSignalCredsFound = shouldSignalCredsFound;
}

void TestableDownloadManager::getCredentials()
{
    if (_shouldSignalCredsFound) {
        emit service.credentialsFound(_token);
    }else{
        emit service.credentialsNotFound();
    }
}

// Test Cases:

void TestDownloadManager::testFetchClickTokenCredentialsNotFound()
{
    _tdm.setShouldSignalCredsFound(false);
    FakeNam::shouldSignalNetworkError = false;
    QSignalSpy spy(&_tdm, SIGNAL(clickTokenFetchError(QString)));
    _tdm.fetchClickToken(TEST_URL);
    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, SCOPE_TEST_TIMEOUT_MSEC);
}

void TestDownloadManager::testFetchClickTokenCredsFoundButNetworkError()
{
    _tdm.setShouldSignalCredsFound(true);
    FakeNam::shouldSignalNetworkError = true;
    QSignalSpy spy(&_tdm, SIGNAL(clickTokenFetchError(QString)));
    _tdm.fetchClickToken(TEST_URL);
    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, SCOPE_TEST_TIMEOUT_MSEC);
}

void TestDownloadManager::testFetchClickTokenSuccess()
{
    _tdm.setShouldSignalCredsFound(true);
    FakeNam::shouldSignalNetworkError = false;
    QSignalSpy spy(&_tdm, SIGNAL(clickTokenFetched(QString)));
    _tdm.fetchClickToken(TEST_URL);
    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, SCOPE_TEST_TIMEOUT_MSEC);
}
