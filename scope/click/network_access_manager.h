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

#ifndef CLICK_NETWORK_ACCESS_MANAGER_H
#define CLICK_NETWORK_ACCESS_MANAGER_H

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QString>

class QByteArray;

namespace click
{
namespace network
{
// We wrap a QNetworkReply to make sure that we can mock it out
// in unit- and integration testing.
class Reply : public QObject
{
    Q_OBJECT

public:
    // A Reply instance takes over ownership of the underlying QNetworkReply.
    explicit Reply(QNetworkReply* reply);
    Reply(const Reply&) = delete;
    virtual ~Reply();

    Reply& operator=(const Reply&) = delete;

    virtual void abort();

    virtual QByteArray readAll();

    virtual QVariant attribute(QNetworkRequest::Attribute code);

    virtual bool hasRawHeader(const QByteArray &headerName);
    virtual QString rawHeader(const QByteArray &headerName);
    virtual QList<QPair<QByteArray, QByteArray>> rawHeaderPairs();

    virtual QString errorString();

signals:
    void finished();
    void error(QNetworkReply::NetworkError);

protected:
    Reply();

private:
    QScopedPointer<QNetworkReply> reply;
};

class AccessManager
{
public:
    AccessManager() = default;
    AccessManager(const AccessManager&) = delete;
    virtual ~AccessManager() = default;

    AccessManager& operator=(const AccessManager&) = delete;

    virtual QSharedPointer<Reply> get(QNetworkRequest& request);
    virtual QSharedPointer<Reply> head(QNetworkRequest& request);
    virtual QSharedPointer<Reply> post(QNetworkRequest& request, QByteArray& data);
};
}
}

#endif // CLICK_NETWORK_ACCESS_MANAGER_H
