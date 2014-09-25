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

namespace
{

static const QString DOWNLOAD_APP_ID_KEY = "app_id";
static const QString DOWNLOAD_COMMAND_KEY = "post-download-command";

static const QString DOWNLOAD_COMMAND = CLICK_INSTALL_HELPER;

static const QString DOWNLOAD_MANAGER_SHA512 = "sha512";
}

struct click::DownloadManager::Private
{
    Private(const QSharedPointer<click::network::AccessManager>& networkAccessManager,
            const QSharedPointer<click::CredentialsService>& credentialsService,
            const QSharedPointer<udm::Manager>& systemDownloadManager)
        : nam(networkAccessManager), credentialsService(credentialsService),
          systemDownloadManager(systemDownloadManager)
    {
    }

    void updateCredentialsFromService()
    {
        credentialsService->getCredentials();
    }

    void invalidateCredentialsFromService()
    {
        credentialsService->invalidateCredentials();
    }

    QSharedPointer<click::network::AccessManager> nam;
    QSharedPointer<click::CredentialsService> credentialsService;
    QSharedPointer<udm::Manager> systemDownloadManager;
    QSharedPointer<click::network::Reply> reply;
    QString downloadUrl;
    QString download_sha512;
    QString package_name;
};

const QByteArray& click::CLICK_TOKEN_HEADER()
{
    static const QByteArray result("X-Click-Token");
    return result;
}

click::DownloadManager::DownloadManager(const QSharedPointer<click::network::AccessManager>& networkAccessManager,
                                        const QSharedPointer<click::CredentialsService>& credentialsService,
                                        const QSharedPointer<udm::Manager>& systemDownloadManager,
                                        QObject *parent)
    : QObject(parent),
      impl(new Private(networkAccessManager, credentialsService, systemDownloadManager))
{
    QMetaObject::Connection c = connect(impl->credentialsService.data(),
                                        &click::CredentialsService::credentialsFound,
                                        this, &click::DownloadManager::handleCredentialsFound);
    if (!c) {
        qDebug() << "failed to connect to credentialsFound";
    }

    c = connect(impl->credentialsService.data(), &click::CredentialsService::credentialsNotFound,
                this, &click::DownloadManager::handleCredentialsNotFound);
    if (!c) {
        qDebug() << "failed to connect to credentialsNotFound";
    }

    // NOTE: using SIGNAL/SLOT macros here because new-style
    // connections are flaky on ARM.
    c = connect(impl->systemDownloadManager.data(), SIGNAL(downloadCreated(Download*)),
                this, SLOT(handleDownloadCreated(Download*)));

    if (!c) {
        qDebug() << "failed to connect to systemDownloadManager::downloadCreated";

    }
}

click::DownloadManager::~DownloadManager(){
}

void click::DownloadManager::startDownload(const QString& downloadUrl, const QString& download_sha512, const QString& package_name)
{
    impl->package_name = package_name;

    // NOTE: using SIGNAL/SLOT macros here because new-style
    // connections are flaky on ARM.
    QObject::connect(this, SIGNAL(clickTokenFetched(QString)),
                     this, SLOT(handleClickTokenFetched(QString)),
                     Qt::UniqueConnection);
    QObject::connect(this, SIGNAL(clickTokenFetchError(QString)),
                     this, SLOT(handleClickTokenFetchError(QString)),
                     Qt::UniqueConnection);
    fetchClickToken(downloadUrl, download_sha512);
}

void click::DownloadManager::handleClickTokenFetched(const QString& clickToken)
{
    QVariantMap metadata;

    QVariant commandline = QVariant(QStringList() << DOWNLOAD_COMMAND << "$file" << impl->package_name);
    metadata[DOWNLOAD_COMMAND_KEY] = commandline;
    metadata[DOWNLOAD_APP_ID_KEY] = impl->package_name;
    metadata["package_name"] = impl->package_name;

    QMap<QString, QString> headers;
    headers[CLICK_TOKEN_HEADER()] = clickToken;

    udm::DownloadStruct downloadStruct(impl->downloadUrl,
                                       impl->download_sha512,
                                       DOWNLOAD_MANAGER_SHA512,
                                       metadata,
                                       headers);

    impl->systemDownloadManager->createDownload(downloadStruct);

}

void click::DownloadManager::handleClickTokenFetchError(const QString& errorMessage)
{
    emit downloadError(errorMessage);
}

void click::DownloadManager::handleDownloadCreated(udm::Download *download)
{
    if (download->isError()) {
        QString error = download->error()->errorString();
        qDebug() << "Received error from ubuntu-download-manager:" << error;
        emit downloadError(error);
    } else {
        download->start();
        emit downloadStarted(download->id());
    }
}

void click::DownloadManager::fetchClickToken(const QString& downloadUrl, const QString& download_sha512)
{
    impl->downloadUrl = downloadUrl;
    impl->download_sha512 = download_sha512;
    impl->updateCredentialsFromService();
}

void click::DownloadManager::handleCredentialsFound(const u1::Token &token)
{
    qDebug() << "Credentials found, signing url " << impl->downloadUrl;

    QString authHeader = token.signUrl(impl->downloadUrl, QStringLiteral("HEAD"));

    QNetworkRequest req;
    req.setRawHeader(QStringLiteral("Authorization").toUtf8(),
                     authHeader.toUtf8());
    req.setUrl(impl->downloadUrl);

    impl->reply = impl->nam->head(req);

    // NOTE: using SIGNAL/SLOT macros here because new-style
    // connections are flaky on ARM.
    QObject::connect(impl->reply.data(), SIGNAL(error(QNetworkReply::NetworkError)),
                     this, SLOT(handleNetworkError(QNetworkReply::NetworkError)));
    QObject::connect(impl->reply.data(), SIGNAL(finished()),
                     this, SLOT(handleNetworkFinished()));
}

void click::DownloadManager::handleCredentialsNotFound()
{
    qDebug() << "No credentials were found.";
    emit credentialsNotFound();
}

void click::DownloadManager::handleNetworkFinished()
{
    QVariant statusAttr = impl->reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if(!statusAttr.isValid()) {
        QString msg("Invalid HTTP response.");
        qDebug() << msg;
        emit clickTokenFetchError(msg);
        return;
    }

    int status = statusAttr.toInt();
    if (status != 200){
        qDebug() << impl->reply->rawHeaderPairs();
        qDebug() << impl->reply->readAll();
        QString msg = QString("HTTP status not OK: %1").arg(status);
        emit clickTokenFetchError(msg);
        return;
    }

    if(!impl->reply->hasRawHeader(CLICK_TOKEN_HEADER())) {
        QString msg = "Response does not contain Click Header";
        qDebug() << msg << "Full response:";
        qDebug() << impl->reply->rawHeaderPairs();
        qDebug() << impl->reply->readAll();

        emit clickTokenFetchError(msg);
        return;
    }

    QString clickTokenHeaderStr = impl->reply->rawHeader(CLICK_TOKEN_HEADER());

    impl->reply.reset();

    emit clickTokenFetched(clickTokenHeaderStr);
}

void click::DownloadManager::handleNetworkError(QNetworkReply::NetworkError error)
{
    switch (error) {
        case QNetworkReply::ContentAccessDenied:
        case QNetworkReply::ContentOperationNotPermittedError:
        case QNetworkReply::AuthenticationRequiredError:
            impl->invalidateCredentialsFromService();
            emit credentialsNotFound();
            break;
        default:
            qDebug() << "error in network request for click token: " << error << impl->reply->errorString();
            emit clickTokenFetchError(QString("Network Error"));
            break;
    }
    impl->reply.reset();
}

void click::DownloadManager::getAllDownloadsWithMetadata(const QString &key, const QString &value,
                                                         MetadataDownloadsListCb callback,
                                                         MetadataDownloadsListCb errback)
{
    impl->systemDownloadManager->getAllDownloadsWithMetadata(key, value, callback, errback);
}

// Downloader
namespace
{
click::DownloadManager& downloadManagerInstance(
        const QSharedPointer<click::network::AccessManager>& networkAccessManager)
{
    static QSharedPointer<click::CredentialsService> ssoService
    {
        new click::CredentialsService()
    };
    static QSharedPointer<Ubuntu::DownloadManager::Manager> udm
    {
        Ubuntu::DownloadManager::Manager::createSessionManager()
    };

    static click::DownloadManager instance(networkAccessManager,
                                           ssoService,
                                           udm);

    return instance;
}
}

click::Downloader::Downloader(const QSharedPointer<click::network::AccessManager>& networkAccessManager)
    : networkAccessManager(networkAccessManager)
{
}

click::Downloader::~Downloader()
{

}

click::DownloadManager& click::Downloader::getDownloadManager()
{
    return downloadManagerInstance(networkAccessManager);
}

void click::Downloader::get_download_progress(std::string package_name, const std::function<void (std::string)>& callback)
{
    auto& dm = getDownloadManager();

    dm.getAllDownloadsWithMetadata(DOWNLOAD_APP_ID_KEY, QString::fromStdString(package_name),
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

namespace
{
class Callback : public QObject
{
    Q_OBJECT

public:
    Callback(const std::function<void (std::pair<std::string, click::InstallError >)>& cb) : cb(cb)
    {
    }

public slots:
    void onDownloadStarted(const QString& downloadId)
    {
        cb(std::make_pair(downloadId.toUtf8().data(), click::InstallError::NoError));

        // We shouldn't do this, but: We have no other indication whether a download finished or not.
        // TODO(tvoss): Remove as soon as a donwload finished signal is available.
        deleteLater();
    }

    void onDownloadError(const QString& errorMessage)
    {
        cb(std::make_pair(errorMessage.toStdString(), click::InstallError::DownloadInstallError));
        deleteLater();
    }

    void onCredentialsError()
    {
        cb(std::make_pair(std::string(), click::InstallError::CredentialsError));
        deleteLater();
    }

private:
    std::function<void (std::pair<std::string, click::InstallError >)> cb;
};
}

void click::Downloader::startDownload(const std::string& url, const std::string& download_sha512, const std::string& package_name,
                                      const std::function<void (std::pair<std::string, click::InstallError >)>& callback)
{
    qt::core::world::enter_with_task([this, callback, url, download_sha512, package_name] ()
    {
        auto& dm = downloadManagerInstance(networkAccessManager);

        // Leverage automatic lifetime mgmt for QObjects here.
        auto cb = new Callback{callback};

        QObject::connect(&dm, &click::DownloadManager::downloadStarted,
                         cb, &Callback::onDownloadStarted);

        QObject::connect(&dm, &click::DownloadManager::credentialsNotFound,
                         cb, &Callback::onCredentialsError);

        QObject::connect(&dm, &click::DownloadManager::downloadError,
                         cb, &Callback::onDownloadError);

        dm.startDownload(QString::fromStdString(url), QString::fromStdString(download_sha512),
                          QString::fromStdString(package_name));
    });
}

#include "download-manager.moc"
