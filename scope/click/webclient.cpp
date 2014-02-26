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
    QSharedPointer<click::network::AccessManager> network_access_manager;
    QSharedPointer<click::CredentialsService> sso;
};

click::web::Service::Service(
    const  QSharedPointer<click::network::AccessManager>& network_access_manager,
    const QSharedPointer<click::CredentialsService>& sso
)
    : impl(new Private{network_access_manager, sso})
{
}

click::web::Service::~Service()
{
}

QSharedPointer<click::web::Response> click::web::Service::call(
    const std::string& iri,
    const click::web::CallParams& params)
{
    return call(iri, click::web::Method::GET, false,
                std::map<std::string, std::string>(), "", params);
}

QSharedPointer<click::web::Response> click::web::Service::call(
    const std::string& iri,
    click::web::Method method,
    bool sign,
    const std::map<std::string, std::string>& headers,
    const std::string& post_data,
    const click::web::CallParams& params)
{
    QUrl url(iri.c_str());
    url.setQuery(params.query);
    QNetworkRequest request(url);
    QByteArray data(post_data.c_str(), post_data.length());
    QSharedPointer<click::network::Reply> reply;

    for (const auto& kv : headers) {
        QByteArray header_name(kv.first.c_str(), kv.first.length());
        QByteArray header_value(kv.second.c_str(), kv.second.length());
        request.setRawHeader(header_name, header_value);
    }

    if (sign) {
        // TODO: Get the credentials, sign the request, and add the header.
    }

    switch (method) {
    case click::web::Method::GET:
        reply = impl->network_access_manager->get(request);
        break;
    case click::web::Method::HEAD:
        reply = impl->network_access_manager->head(request);
        break;
    case click::web::Method::POST:
        reply = impl->network_access_manager->post(request, data);
        break;
    default:
        qCritical() << "Unhandled request method:" << method;
        break;
    }

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
