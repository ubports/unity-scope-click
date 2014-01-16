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

namespace ClickScope {


DownloadManager::DownloadManager(QObject *parent) :
    QObject(parent)
{
    QObject::connect(&service, SIGNAL(credentialsFound(const Token&)),
                     this, SLOT(handleCredentialsFound(Token)));
    QObject::connect(&service, SIGNAL(credentialsNotFound()),
                     this, SLOT(handleCredentialsNotFound()));
    QObject::connect(&nam, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(handleNetworkFinished(QNetworkReply*)));

}

DownloadManager::~DownloadManager(){
}

void DownloadManager::getCredentials()
{
    service.getCredentials();
}

void DownloadManager::fetchClickToken(QString downloadUrl)
{
    getCredentials();
    _downloadUrl = downloadUrl;
}

void DownloadManager::handleCredentialsFound(UbuntuOne::Token token)
{
    qDebug() << "Credentials found, signing url " << _downloadUrl;

    QString authHeader = token.signUrl(_downloadUrl, QStringLiteral("HEAD"));

    qDebug() << "URL Signed, authHeader is:" << authHeader; // TODO: remove this log

    QNetworkRequest req;
    req.setRawHeader(QStringLiteral("Authorization").toUtf8(),
                     authHeader.toUtf8());
    req.setUrl(_downloadUrl);
    nam.get(req);
}
 
void DownloadManager::handleCredentialsNotFound()
{
    qDebug() << "No credentials were found.";
    emit clickTokenFetchError(QString("No creds found"));
}

void DownloadManager::handleNetworkFinished(QNetworkReply *reply)
{
    // TODO: actually get the header
    QString clickTokenHeader;
    QVariant statusAttr = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if(!statusAttr.isValid()) {
        qDebug() << "Invalid HTTP response.";
        return;
    }

    int status = statusAttr.toInt();
    qDebug() << "HTTP Status " << status;

    if (status != 200){
        qDebug() << reply->rawHeaderPairs();
    }

    qDebug() << reply->readAll();
        
    emit clickTokenFetched(clickTokenHeader);
}

} // namespace ClickScope
