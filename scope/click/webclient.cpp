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
#include "QDebug"
#include "webclient.h"

#include "network_access_manager.h"

void click::web::CallParams::add(const std::string& key, const std::string& value)
{
    query.addQueryItem(key.c_str(), value.c_str());
}

bool click::web::CallParams::operator==(const CallParams &other) const
{
    return (this->query == other.query);
}


struct click::web::Service::Private
{
    std::string base_url;
    QSharedPointer<click::network::AccessManager> network_access_manager;
};

click::web::Service::Service(const std::string& base,
        const QSharedPointer<click::network::AccessManager>& network_access_manager)
    : impl(new Private{base, network_access_manager})
{
}

click::web::Service::~Service()
{
}

QSharedPointer<click::web::Response> click::web::Service::call(const std::string &path, const click::web::CallParams& params)
{
    QUrl url((impl->base_url+path).c_str());
    url.setQuery(params.query);

    QNetworkRequest request(url);
    auto reply = impl->network_access_manager->get(request);

    return QSharedPointer<click::web::Response>(new click::web::Response(reply));
}

click::web::Response::Response(const QSharedPointer<click::network::Reply>& reply, QObject* parent)
    : QObject(parent),
      reply(reply)
{
    connect(reply.data(), &click::network::Reply::finished, this, &web::Response::replyFinished);
}


void click::web::Response::replyFinished()
{
    emit finished(reply->readAll());
}

click::web::Response::~Response()
{
}
