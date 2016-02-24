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

#include "download-manager.h"

#include <QDebug>
#include <QObject>
#include <QString>
#include <QStringBuilder>
#include <QTimer>

#include <click/qtbridge.h>

namespace u1 = UbuntuOne;
#include <click/ubuntuone_credentials.h>
#include <token.h>

namespace udm = Ubuntu::DownloadManager;
#include <ubuntu/download_manager/download_struct.h>
#include <ubuntu/download_manager/downloads_list.h>
#include <ubuntu/download_manager/download.h>
#include <ubuntu/download_manager/error.h>

namespace click
{

static const QString DOWNLOAD_APP_ID_KEY = "app_id";
static const QString DOWNLOAD_COMMAND_KEY = "post-download-command";

static const QString DOWNLOAD_COMMAND = CLICK_INSTALL_HELPER;

static const QString DOWNLOAD_MANAGER_SHA512 = "sha512";

const QByteArray& CLICK_TOKEN_HEADER()
{
    static const QByteArray result("X-Click-Token");
    return result;
}

DownloadManager::DownloadManager(const QSharedPointer<click::web::Client>& client,
                                 const QSharedPointer<udm::Manager>& manager) :
    client(client),
    dm(manager)
{
}

DownloadManager::~DownloadManager()
{
}

void DownloadManager::get_progress(const std::string& package_name,
                                   const std::function<void (std::string)>& callback)
{
    dm->getAllDownloadsWithMetadata(DOWNLOAD_APP_ID_KEY,
                                    QString::fromStdString(package_name),
                                    [callback, package_name](const QString& /*key*/, const QString& /*value*/, DownloadsList* downloads_list){
        // got downloads matching metadata
        std::string object_path;
        auto downloads = downloads_list->downloads();
        if (downloads.size() > 0) {
            auto download = downloads.at(0);
            object_path = download->id().toStdString();
        }
        qDebug() << "Found object path" << QString::fromStdString(object_path)
                 << "for package" << QString::fromStdString(package_name);
        if (downloads.size() > 1) {
            qWarning() << "More than one download with the same object path";
        }
        callback(object_path);
    }, [callback, package_name](const QString& /*key*/, const QString& /*value*/, DownloadsList* /*downloads_list*/){
        // no downloads found
        qDebug() << "No object path found for package" << QString::fromStdString(package_name);
        callback("");
    });
}

click::web::Cancellable DownloadManager::start(const std::string& url,
                                               const std::string& download_sha512,
                                               const std::string& package_name,
                                               const std::function<void (std::string, Error)>& callback)
{
    QSharedPointer<click::web::Response> response = client->call
        (url, "HEAD", true);

    QObject::connect(response.data(), &click::web::Response::finished,
                [=](QString) {
                    auto status = response->get_status_code();
                    if (status == 200) {
                        auto clickToken = response->get_header(CLICK_TOKEN_HEADER().data());
                        QVariantMap metadata;

                        QVariant commandline = QVariant(QStringList() << DOWNLOAD_COMMAND << "$file" << package_name.c_str());
                        metadata[DOWNLOAD_COMMAND_KEY] = commandline;
                        metadata[DOWNLOAD_APP_ID_KEY] = package_name.c_str();
                        metadata["package_name"] = package_name.c_str();

                        QMap<QString, QString> headers;
                        headers[CLICK_TOKEN_HEADER()] = clickToken.c_str();

                        udm::DownloadStruct downloadStruct(url.c_str(),
                                                           download_sha512.c_str(),
                                                           DOWNLOAD_MANAGER_SHA512,
                                                           metadata,
                                                           headers);

                        QObject::connect(dm.data(), &udm::Manager::downloadCreated,
                                         [callback](Download* download) {
                                             if (download->isError()) {
                                                 auto error = download->error()->errorString().toUtf8().data();
                                                 qDebug() << "Received error from ubuntu-download-manager:" << error;
                                                 callback(error, Error::DownloadInstallError);
                                             } else {
                                                 download->start();
                                                 callback(download->id().toUtf8().data(), Error::NoError);
                                             }
                                         });
                        dm->createDownload(downloadStruct);
                    } else {
                        std::string error{"Unhandled HTTP response code: "};
                        error += status;
                        callback(error, Error::DownloadInstallError);
                    }
                });
    QObject::connect(response.data(), &click::web::Response::error,
                     [=](QString error, int error_code) {
                         qDebug() << QStringLiteral("Network error (%1) fetching click token for:").arg(error_code) << package_name.c_str();
                         switch(error_code) {
                         case 401:
                         case 403:
                             client->invalidateCredentials();
                             callback(error.toUtf8().data(), Error::CredentialsError);
                             break;
                         default:
                             callback(error.toUtf8().data(), Error::DownloadInstallError);
                         }
                     });

    return click::web::Cancellable(response);
}

} // namespace click
