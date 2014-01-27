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

#ifndef FAKE_NAM_H
#define FAKE_NAM_H

#include <network-access-manager.h>

#include <gmock/gmock.h>

#include <QObject>
#include <QNetworkReply>

class MockNetworkReply : public click::network::Reply
{
public slots:
    void sendFinished();
    void sendError();
    void abort();

public:
    MOCK_METHOD0(readAll, QByteArray());
    MOCK_METHOD1(attribute, QVariant(QNetworkRequest::Attribute));
    MOCK_METHOD1(hasRawHeader, bool(const QByteArray&));
    MOCK_METHOD1(rawHeader, QString(const QByteArray &headerName));

    typedef QList<QPair<QByteArray, QByteArray>> ResultType;
    MOCK_METHOD0(rawHeaderPairs, ResultType());
    MOCK_METHOD0(errorString, QString());
};

struct MockNetworkAccessManager : public click::network::AccessManager
{
    MockNetworkAccessManager()
    {
    }

    MOCK_METHOD1(get, click::network::Reply*(QNetworkRequest&));
    MOCK_METHOD1(head, click::network::Reply*(QNetworkRequest&));
    MOCK_METHOD2(post, click::network::Reply*(QNetworkRequest&, QByteArray&));

    static QList<QByteArray> scripted_responses;
    static QList<QNetworkRequest> performed_get_requests;
    static QList<QNetworkRequest> performed_head_requests;
    static bool shouldSignalNetworkError;
};

#endif // FAKE_NAM_H
