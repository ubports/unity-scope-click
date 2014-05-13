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
#include <QDebug>

#include "configuration.h"
#include "webclient.h"
#include "smartconnect.h"

#include "network_access_manager.h"

void click::web::CallParams::add(const std::string& key, const std::string& value)
{
    query.addQueryItem(key.c_str(), value.c_str());
}

bool click::web::CallParams::operator==(const CallParams &other) const
{
    return (this->query == other.query);
}


struct click::web::Client::Private
{
    Private(const QSharedPointer<click::network::AccessManager> nam)
        : network_access_manager(nam)
    {
    }

    QSharedPointer<click::network::AccessManager> network_access_manager;
    QSharedPointer<click::CredentialsService> sso;

    void setCredentialsService(const QSharedPointer<click::CredentialsService>& sso)
    {
        this->sso = sso;
    }
};

click::web::Client::Client(const  QSharedPointer<click::network::AccessManager>& network_access_manager)
    : impl(new Private{network_access_manager})
{
}

click::web::Client::~Client()
{
}

QSharedPointer<click::web::Response> click::web::Client::call(
    const std::string& iri,
    const click::web::CallParams& params)
{
    return call(iri, "GET", false,
                std::map<std::string, std::string>(), "", params);
}

QSharedPointer<click::web::Response> click::web::Client::call(
    const std::string& iri,
    const std::string& method,
    bool sign,
    const std::map<std::string, std::string>& headers,
    const std::string& data,
    const click::web::CallParams& params)
{
    QUrl url(iri.c_str());
    url.setQuery(params.query);
    QSharedPointer<QNetworkRequest> request(new QNetworkRequest(url));
    QSharedPointer<QBuffer> buffer(new QBuffer());
    buffer->setData(data.c_str(), data.length());

    // Set the Accept-Language header for all requests.
    request->setRawHeader(ACCEPT_LANGUAGE_HEADER.c_str(),
                          Configuration().get_accept_languages().c_str());

    for (const auto& kv : headers) {
        QByteArray header_name(kv.first.c_str(), kv.first.length());
        QByteArray header_value(kv.second.c_str(), kv.second.length());
        request->setRawHeader(header_name, header_value);
    }

    QSharedPointer<click::web::Response> responsePtr = QSharedPointer<click::web::Response>(new click::web::Response(request, buffer));

    auto doConnect = [=]() {
        QByteArray verb(method.c_str(), method.length());
        auto reply = impl->network_access_manager->sendCustomRequest(*request,
                                                                verb,
                                                                buffer.data());
        responsePtr->setReply(reply);
    };

    if (sign && !impl->sso.isNull()) {
        click::utils::SmartConnect sc(responsePtr.data());
        sc.connect(impl->sso.data(), &click::CredentialsService::credentialsFound,
                   [=](const UbuntuOne::Token& token) {
                       QString auth_header = token.signUrl(url.toString(),
                                                           method.c_str());
                       qDebug() << "Signed URL:" << request->url().toString();
                       request->setRawHeader(AUTHORIZATION_HEADER.c_str(), auth_header.toUtf8());
                       impl->sso.clear();
                       doConnect();
                   });
        sc.connect(impl->sso.data(), &click::CredentialsService::credentialsNotFound,
                   [this]() {
                       // TODO: Need to handle and propagate error conditons.
                       impl->sso.clear();
                   });
        // TODO: Need to handle error signal once in CredentialsService.
        impl->sso->getCredentials();
    } else {
        doConnect();
    }


    return responsePtr;
}

void click::web::Client::setCredentialsService(const QSharedPointer<click::CredentialsService>& sso)
{
    impl->setCredentialsService(sso);
}

click::web::Response::Response(const QSharedPointer<QNetworkRequest>& request,
                               const QSharedPointer<QBuffer>& buffer,
                               QObject* parent)
    : QObject(parent),
      request(request),
      buffer(buffer)
{
}

void click::web::Response::setReply(QSharedPointer<network::Reply> reply)
{
    this->reply = reply;
    auto sc = new click::utils::SmartConnect(reply.data());
    sc->connect(this->reply.data(), &click::network::Reply::finished,
                [this](){replyFinished();});
    sc->connect(this->reply.data(), &click::network::Reply::error,
                [this](QNetworkReply::NetworkError err){errorHandler(err);});
}

void click::web::Response::replyFinished()
{
    auto response = reply->readAll();
    qDebug() << "got response" << response.toPercentEncoding(" ");
    emit finished(response);
}

void click::web::Response::errorHandler(QNetworkReply::NetworkError network_error)
{
    auto message = reply->errorString() + QString(" (%1)").arg(network_error);
    qCritical() << "Error response body:" << reply->readAll();
    qDebug() << "emitting error: " << message;
    emit error(message);
}

void click::web::Response::abort()
{
    reply->abort();
}

click::web::Response::~Response()
{
}
