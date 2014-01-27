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

#include <QDebug>
#include <QObject>
#include <QString>
#include <QTimer> 

#include "ssoservice.h"
#include "token.h"

#include "download-manager.h"

using namespace UbuntuOne;

click::DownloadManager::DownloadManager(QObject *parent) :
    QObject(parent)
{
    QMetaObject::Connection c = connect(&service, SIGNAL(credentialsFound(Token)),
                                        this, SLOT(handleCredentialsFound(const Token&)));
    if(!c){
        qDebug() << "failed to connect to credentialsFound";
    }

    c = connect(&service, SIGNAL(credentialsNotFound()),
                         this, SLOT(handleCredentialsNotFound()));
    if(!c){
        qDebug() << "failed to connect to credentialsNotFound";
    }
}

click::DownloadManager::~DownloadManager(){
    if (_reply != nullptr) {
        _reply->abort();
        _reply->deleteLater();
    }
}

void click::DownloadManager::getCredentials()
{
    service.getCredentials();
}

void click::DownloadManager::fetchClickToken(QString downloadUrl)
{
    getCredentials();
    _downloadUrl = downloadUrl;
}

void click::DownloadManager::handleCredentialsFound(const Token &token)
{
    qDebug() << "Credentials found, signing url " << _downloadUrl;

    QString authHeader = token.signUrl(_downloadUrl, QStringLiteral("HEAD"));

    QNetworkRequest req;
    req.setRawHeader(QStringLiteral("Authorization").toUtf8(),
                     authHeader.toUtf8());
    req.setUrl(_downloadUrl);

    _reply = nam.head(req);
    QObject::connect(_reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError )>(&QNetworkReply::error),
                     this, &DownloadManager::handleNetworkError);
    QObject::connect(_reply, &QNetworkReply::finished,
                     this, &DownloadManager::handleNetworkFinished);
}
 
void click::DownloadManager::handleCredentialsNotFound()
{
    qDebug() << "No credentials were found.";
    emit clickTokenFetchError(QString("No creds found"));
}

void click::DownloadManager::handleNetworkFinished()
{
    QVariant statusAttr = _reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if(!statusAttr.isValid()) {
        QString msg("Invalid HTTP response.");
        qDebug() << msg;
        emit clickTokenFetchError(msg);
        return;
    }

    int status = statusAttr.toInt();
    if (status != 200){
        qDebug() << _reply->rawHeaderPairs();
        qDebug() << _reply->readAll();
        QString msg = QString("HTTP status not OK: %1").arg(status);
        emit clickTokenFetchError(msg);
        return;
    }

    if(!_reply->hasRawHeader(CLICK_TOKEN_HEADER)) {
        QString msg = "Response does not contain Click Header";
        qDebug() << msg << "Full response:";
        qDebug() << _reply->rawHeaderPairs();
        qDebug() << _reply->readAll();

        emit clickTokenFetchError(msg);
        return;
    }
    QString clickTokenHeaderStr = _reply->rawHeader(CLICK_TOKEN_HEADER);

    _reply->deleteLater();
    _reply = nullptr;
    emit clickTokenFetched(clickTokenHeaderStr);
}

void click::DownloadManager::handleNetworkError(QNetworkReply::NetworkError error)
{
    qDebug() << "error in network request for click token: " << error << _reply->errorString();
    _reply->deleteLater();
    _reply = nullptr;
    emit clickTokenFetchError(QString("Network Error"));
}
