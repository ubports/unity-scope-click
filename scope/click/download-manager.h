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

#ifndef CLICK_DOWNLOAD_MANAGER_H
#define CLICK_DOWNLOAD_MANAGER_H

#include <click/config.h>

#include "network_access_manager.h"
#include "ubuntuone_credentials.h"

#include <QDebug>
#include <QNetworkReply>
#include <QObject>
#include <QString>

#include <ubuntu/download_manager/manager.h>

namespace UbuntuOne
{
class Token;
}

namespace click
{
// The dbus-send command to refresh the search results in the dash.
static const QString DBUSSEND_COMMAND = QStringLiteral("dbus-send /com/canonical/unity/scopes com.canonical.unity.scopes.InvalidateResults string:clickscope");

const QByteArray& CLICK_TOKEN_HEADER();

class DownloadManager : public QObject
{
    Q_OBJECT

public:
    DownloadManager(const QSharedPointer<click::network::AccessManager>& networkAccessManager,
                    const QSharedPointer<click::CredentialsService>& ssoService,
                    const QSharedPointer<Ubuntu::DownloadManager::Manager>& systemDownloadManager,
                    QObject *parent = 0);
    virtual ~DownloadManager();

public slots:
    virtual void startDownload(const QString& downloadUrl, const QString& appId);
    virtual void fetchClickToken(const QString& downloadUrl);

signals:

    void credentialsNotFound();
    void downloadStarted(const QString& downloadObjectPath);
    void downloadError(const QString& errorMessage);
    void clickTokenFetched(const QString& clickToken);
    void clickTokenFetchError(const QString& errorMessage);

protected slots:
    virtual void handleCredentialsFound(const UbuntuOne::Token &token);
    virtual void handleCredentialsNotFound();
    virtual void handleNetworkFinished();
    virtual void handleNetworkError(QNetworkReply::NetworkError error);
    virtual void handleDownloadCreated(Ubuntu::DownloadManager::Download *download);
    virtual void handleClickTokenFetched(const QString& clickToken);
    virtual void handleClickTokenFetchError(const QString& errorMessage);

protected:
    struct Private;
    QScopedPointer<Private> impl;
};

enum class InstallError {NoError, CredentialsError, DownloadInstallError};

class Downloader
{
public:
    Downloader(const QSharedPointer<click::network::AccessManager>& networkAccessManager);
    void get_download_progress(std::string package_name, const std::function<void (std::string)>& callback);
    void startDownload(std::string url, std::string package_name,
                       const std::function<void (std::pair<std::string, InstallError>)>& callback);
private:
    QSharedPointer<click::network::AccessManager> networkAccessManager;
};

}

#endif /* CLICK_DOWNLOAD_MANAGER_H */
