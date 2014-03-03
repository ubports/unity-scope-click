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

#include <QBuffer>
#include <QCoreApplication>
#include <QDebug>

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
    Private(const QSharedPointer<click::network::AccessManager> nam,
            const QSharedPointer<click::CredentialsService> sso)
        : network_access_manager(nam),
          sso(sso)
    {
    }

    QSharedPointer<click::network::AccessManager> network_access_manager;
    QSharedPointer<click::CredentialsService> sso;
    QSharedPointer<UbuntuOne::Token> token;
};

click::web::Service::Service(
    const  QSharedPointer<click::network::AccessManager>& network_access_manager,
    const QSharedPointer<click::CredentialsService>& sso
)
    : impl(new Private{network_access_manager, sso})
{
    QObject::connect(sso.data(), &click::CredentialsService::credentialsFound,
                     [&](const UbuntuOne::Token& token) {
                         impl->token.reset(new UbuntuOne::Token(token));
                     });
}

click::web::Service::~Service()
{
}

QSharedPointer<click::web::Response> click::web::Service::call(
    const std::string& iri,
    const click::web::CallParams& params)
{
    return call(iri, "GET", false,
                std::map<std::string, std::string>(), "", params);
}

QSharedPointer<click::web::Response> click::web::Service::call(
    const std::string& iri,
    const std::string& method,
    bool sign,
    const std::map<std::string, std::string>& headers,
    const std::string& data,
    const click::web::CallParams& params)
{
    QUrl url(iri.c_str());
    url.setQuery(params.query);
    QNetworkRequest request(url);
    QSharedPointer<QBuffer> buffer(new QBuffer());
    buffer->setData(data.c_str(), data.length());
    QSharedPointer<click::network::Reply> reply;

    for (const auto& kv : headers) {
        QByteArray header_name(kv.first.c_str(), kv.first.length());
        QByteArray header_value(kv.second.c_str(), kv.second.length());
        request.setRawHeader(header_name, header_value);
    }

    if (sign) {
        impl->sso->getCredentials();
        while (impl->token.isNull()) {
            QCoreApplication::processEvents();
        }
        QString auth_header;
        switch (method) {
        case click::web::Method::GET:
            auth_header = impl->token->signUrl(url.toString(), "GET");
            break;
        case click::web::Method::HEAD:
            auth_header = impl->token->signUrl(url.toString(), "HEAD");
            break;
        case click::web::Method::POST:
            auth_header = impl->token->signUrl(url.toString(), "POST");
            break;
        case click::web::Method::PUT:
            auth_header = impl->token->signUrl(url.toString(), "PUT");
            break;
        default:
            qCritical() << "Cannot sign URL for unknown method:" << method;
            break;
        }
        request.setRawHeader("Authorization", auth_header.toUtf8());
        impl->token.clear();
    }

    QByteArray verb(method.c_str(), method.length());
    reply = impl->network_access_manager->sendCustomRequest(request,
                                                            verb,
                                                            buffer.data());

    return QSharedPointer<click::web::Response>(new click::web::Response(reply, buffer));
}

click::web::Response::Response(const QSharedPointer<click::network::Reply>& reply, const QSharedPointer<QBuffer>& buffer, QObject* parent)
    : QObject(parent),
      reply(reply),
      buffer(buffer)
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
