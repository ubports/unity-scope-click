/*
 * Copyright (C) 2014-2016 Canonical Ltd.
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

#include <QDebug>
#include <QNetworkReply>
#include <QObject>
#include <QString>

#include <click/webclient.h>

#include <ubuntu/download_manager/manager.h>

using Ubuntu::DownloadManager::Download;

namespace UbuntuOne
{
class Token;
}

namespace click
{
// The dbus-send command to refresh the search results in the dash.
static const QString REFRESH_SCOPE_COMMAND = QStringLiteral("dbus-send /com/canonical/unity/scopes com.canonical.unity.scopes.InvalidateResults string:%1");
static const QString APPS_SCOPE_ID = QStringLiteral("clickscope");
static const QString STORE_SCOPE_ID = QStringLiteral("com.canonical.scopes.clickstore");

const QByteArray& CLICK_TOKEN_HEADER();


class DownloadManager
{
public:
    enum class Error {NoError, CredentialsError, DownloadInstallError};

    DownloadManager(const QSharedPointer<click::web::Client>& client,
                    const QSharedPointer<Ubuntu::DownloadManager::Manager>& manager);
    virtual ~DownloadManager();

    virtual void get_progress(const std::string& package_name,
                              const std::function<void (std::string)>& callback);
    virtual click::web::Cancellable start(const std::string& url,
                                          const std::string& download_sha512,
                                          const std::string& package_name,
                                          const std::function<void (std::string,
                                                                    Error)>& callback);

protected:
    QSharedPointer<click::web::Client> client;
    QSharedPointer<Ubuntu::DownloadManager::Manager> dm;
};

}

#endif /* CLICK_DOWNLOAD_MANAGER_H */
