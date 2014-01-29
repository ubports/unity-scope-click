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

#ifndef CLICK_WEBCLIENT_H
#define CLICK_WEBCLIENT_H

#include <QObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSharedPointer>
#include <QUrlQuery>

namespace click
{
namespace network
{
class AccessManager;
class Reply;
}
namespace web
{
class Service;

class CallParams : public QObject
{
    Q_OBJECT
    QUrlQuery query;
    friend class Service;
public:
    void add(const QString& key, const QString& value)
    {
        query.addQueryItem(key, value);
    }
};

class Response : public QObject
{
    Q_OBJECT

public:
    explicit Response(const QSharedPointer<click::network::Reply>& reply, QObject* parent=0);
    ~Response();

private slots:
    void replyFinished();

signals:
    void finished(QString result);

private:
    QSharedPointer<click::network::Reply> reply;
};

class Service
{
public:
    Service();
    Service(const QString& base,
            const QSharedPointer<click::network::AccessManager>& networkAccessManager);
    ~Service();

    QSharedPointer<Response> call(const QString& path, const CallParams& params = CallParams());
private:
    struct Private;
    QScopedPointer<Private> impl;
};
}
}

#endif // CLICK_WEBCLIENT_H
