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

#include <QtGlobal>
#include <QDebug>
#include <QTimer>

#include <fake_nam.h>

#ifdef USE_FAKE_NAM

QList<QByteArray> FakeNam::scripted_responses;
QList<QNetworkRequest> FakeNam::performed_get_requests;
QList<QNetworkRequest> FakeNam::performed_head_requests;
bool FakeNam::shouldSignalNetworkError = false;

FakeReply* FakeNam::get(QNetworkRequest &request)
{
    FakeReply* reply = new FakeReply();
    performed_get_requests.append(request);
    QTimer::singleShot(0, reply, SLOT(sendFinished()));
    return reply;
}

FakeReply* FakeNam::head(QNetworkRequest &request)
{
    FakeReply* reply = new FakeReply();
    performed_head_requests.append(request);
    if (shouldSignalNetworkError) {
        QTimer::singleShot(0, reply, SLOT(sendError()));
    }else{
        QTimer::singleShot(0, reply, SLOT(sendFinished()));
    }
    return reply;
}


// --- FakeReply methods ---

void FakeReply::sendFinished()
{
    emit finished();
}

void FakeReply::sendError()
{
    emit error(FakeReply::BogusErrorForTests);
}

QVariant FakeReply::attribute(QNetworkRequest::Attribute code)
{
    Q_UNUSED(code);
    return QVariant(200);
}

bool FakeReply::hasRawHeader(const QByteArray &headerName)
{
    Q_UNUSED(headerName);
    return true;
}

QString FakeReply::rawHeader(const QByteArray &headerName)
{
    Q_UNUSED(headerName);
    return "Fake click header value";
}

QString FakeReply::rawHeaderPairs()
{
    return "Not really rawHeaderPairs";
}

QString FakeReply::errorString()
{
    return "Bogus error string for tests";
}

QByteArray FakeReply::readAll()
{
    Q_ASSERT_X(!FakeNam::scripted_responses.empty(),
               "FakeReply::readAll", "Not enough scripted responses");
    return QByteArray(FakeNam::scripted_responses.takeFirst());
}

#endif // USE_FAKE_NAM
