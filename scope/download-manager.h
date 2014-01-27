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

#ifndef DOWNLOAD_MANAGER_H
#define DOWNLOAD_MANAGER_H

#include <Config.h>

#include <QDebug>
#include <QNetworkReply>
#include <QObject>
#include <QString>

#include <ssoservice.h>
#include <token.h>
#include <requests.h>
#include <errormessages.h>

#ifdef USE_FAKE_NAM
#include <tests/fake_nam.h>
#endif

using namespace UbuntuOne;

namespace click {

static const QByteArray CLICK_TOKEN_HEADER = QByteArray("X-Click-Token");

class DownloadManager : public QObject
{
    Q_OBJECT

public:
    explicit DownloadManager(QObject *parent = 0);
    ~DownloadManager();

public slots:
    void fetchClickToken(QString downloadUrl);

signals:
    void clickTokenFetched(QString clickToken);
    void clickTokenFetchError(QString errorMessage);

private slots:
    void handleCredentialsFound(const Token &token);
    void handleCredentialsNotFound();
    void handleNetworkFinished();
    void handleNetworkError(QNetworkReply::NetworkError error);

protected:
    virtual void getCredentials();
    
    UbuntuOne::SSOService service;
    QNetworkAccessManager nam;
    QNetworkReply *_reply = nullptr;
    QString _downloadUrl;

};
}

#endif /* DOWNLOAD_MANAGER_H */
