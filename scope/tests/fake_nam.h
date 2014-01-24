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

#ifndef _FAKE_NAM_H_
#define _FAKE_NAM_H_

#ifdef USE_FAKE_NAM

#include <QObject>
#include <QNetworkReply>

class FakeReply : public QObject
{
    Q_OBJECT
public:
    enum NetworkError {
        NoError = 0,
        BogusErrorForTests = 499
    };

public slots:
    void sendFinished();
    void sendError();
    void abort() {};
signals:
    void finished();
    void error(FakeReply::NetworkError);
public:
    QByteArray readAll();
    QVariant attribute(QNetworkRequest::Attribute code);
    bool hasRawHeader(const QByteArray &headerName);
    QString rawHeader(const QByteArray &headerName);
    QString rawHeaderPairs();
    QString errorString();
};

class FakeNam : public QObject
{
    Q_OBJECT
public:
    FakeReply* get(QNetworkRequest& request);
    FakeReply* head(QNetworkRequest& request);
    FakeReply* post(QNetworkRequest& request, QByteArray& data);
    static QList<QByteArray> scripted_responses;
    static QList<QNetworkRequest> performed_get_requests;
    static QList<QNetworkRequest> performed_head_requests;
    static bool shouldSignalNetworkError;
};

#define QNetworkAccessManager FakeNam
#define QNetworkReply FakeReply

#endif // USE_FAKE_NAM
#endif // _FAKE_NAM_H_
