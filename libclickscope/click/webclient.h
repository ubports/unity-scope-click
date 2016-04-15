/*
 * Copyright (C) 2014-2015 Canonical Ltd.
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

#include <QBuffer>
#include <QObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSharedPointer>
#include <QUrlQuery>

#include <click/ubuntuone_credentials.h>

namespace click
{
namespace network
{
class AccessManager;
class Reply;
}
namespace web
{

const std::string ACCEPT_LANGUAGE_HEADER ="Accept-Language";
const std::string AUTHORIZATION_HEADER = "Authorization";
const std::string CONTENT_TYPE_HEADER = "Content-Type";
const std::string DEVICE_ID_HEADER = "X-Device-Id";

const std::string CONTENT_TYPE_JSON = "application/json";

class Client;

class CallParams
{
    QUrlQuery query;
    friend class Client;
public:
    void add(const std::string& key, const std::string& value);
    std::string operator[](const std::string& key) const;
    bool operator==(const CallParams &other) const;
};

class Response : public QObject
{
    Q_OBJECT

public:
    Response(const QSharedPointer<QNetworkRequest>& request,
             const QSharedPointer<QBuffer>& buffer,
             QObject* parent=0);
    void setReply(QSharedPointer<click::network::Reply> reply);
    virtual void abort();
    virtual ~Response();

    virtual bool has_header(const std::string& header) const;
    virtual std::string get_header(const std::string& header) const;
    virtual int get_status_code() const;

public slots:
    void replyFinished();
    void errorHandler(QNetworkReply::NetworkError network_error);

signals:
    void finished(QByteArray result);
    void error(QString description, int error_code);

private:
    QSharedPointer<click::network::Reply> reply;
    QSharedPointer<QNetworkRequest> request;
    QSharedPointer<QBuffer> buffer;
};

class Cancellable
{
protected:
    QSharedPointer<click::web::Response> response;
public:
    Cancellable() {}
    Cancellable(QSharedPointer<click::web::Response> response) : response(response) {}
    virtual void cancel() { if (response) {response->abort();} }
    virtual ~Cancellable() {}
};

class Client
{
public:
    Client(const QSharedPointer<click::network::AccessManager>& networkAccessManager);
    virtual ~Client();

    virtual QSharedPointer<Response> call(
        const std::string& iri,
        const CallParams& params = CallParams());
    virtual QSharedPointer<Response> call(
        const std::string& iri,
        const std::string& method,
        bool sign = true,
        const std::map<std::string, std::string>& headers = std::map<std::string, std::string>(),
        const std::string& data = "",
        const CallParams& params = CallParams());
    void setCredentialsService(const QSharedPointer<click::CredentialsService>& sso);
    virtual void invalidateCredentials();

private:
    struct Private;
    QScopedPointer<Private> impl;
};
}
}

#endif // CLICK_WEBCLIENT_H
