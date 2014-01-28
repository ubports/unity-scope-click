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

#include "network_access_manager.h"

click::network::Reply::Reply(QNetworkReply* reply) : reply(reply)
{
    connect(this->reply.data(),
            &QNetworkReply::finished,
            this,
            &Reply::finished);

    /*connect(this->reply.data(),
            &QNetworkReply::error,
            this,
            &Reply::error);*/
}

click::network::Reply::~Reply()
{
}

QByteArray click::network::Reply::readAll()
{
    return reply->readAll();
}

QVariant click::network::Reply::attribute(QNetworkRequest::Attribute code)
{
    return reply->attribute(code);
}

bool click::network::Reply::hasRawHeader(const QByteArray &headerName)
{
    return reply->hasRawHeader(headerName);
}

QString click::network::Reply::rawHeader(const QByteArray &headerName)
{
    return reply->rawHeader(headerName);
}

QList<QPair<QByteArray, QByteArray>> click::network::Reply::rawHeaderPairs()
{
    return reply->rawHeaderPairs();
}

QString click::network::Reply::errorString()
{
    return reply->errorString();
}

click::network::Reply::Reply()
{
}

namespace
{
QNetworkAccessManager instance;
}

click::network::Reply* click::network::AccessManager::get(QNetworkRequest& request)
{
    return new click::network::Reply(instance.get(request));
}

click::network::Reply* click::network::AccessManager::head(QNetworkRequest& request)
{
    return new click::network::Reply(instance.head(request));
}

click::network::Reply* click::network::AccessManager::post(QNetworkRequest& request, QByteArray& data)
{
    return new click::network::Reply(instance.post(request, data));
}
