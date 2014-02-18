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

#include "qtbridge.h"

namespace u1 = UbuntuOne;
#include <ubuntuone_credentials.h>
#include <token.h>

namespace udm = Ubuntu::DownloadManager;
#include <ubuntu/download_manager/download_struct.h>
#include <ubuntu/download_manager/download.h>
#include <ubuntu/download_manager/error.h>

namespace
{

static const QString DOWNLOAD_APP_ID_KEY = "app_id";
static const QString DOWNLOAD_COMMAND_KEY = "post-download-command";

// Pass two commands using sh -c because UDM expects a single
// executable.
// Use a single string for both commands because the
// list is eventually given to QProcess, which adds quotes around each
// argument, while -c expects a single quoted string.
// Then, use the $0 positional variable in the pkcon command to let us
// pass "$file" as a separate list element, which UDM requires for
// substitution.
static const QString DOWNLOAD_COMMAND = QStringLiteral("pkcon -p install-local $0 && ")
    % "dbus-send" % " /com/canonical/unity/scopes" % " com.canonical.unity.scopes.InvalidateResults"
    % " string:clickscope";
static const QVariant DOWNLOAD_CMDLINE = QVariant(QStringList()
                                                  << "/bin/sh" << "-c" <<
                                                  DOWNLOAD_COMMAND << "$file");

static const QString DOWNLOAD_MANAGER_DO_NOT_HASH = "";
static const QString DOWNLOAD_MANAGER_IGNORE_HASH_ALGORITHM = "";
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

    QSharedPointer<click::network::AccessManager> nam;
    QSharedPointer<click::CredentialsService> credentialsService;
    QSharedPointer<udm::Manager> systemDownloadManager;
    QSharedPointer<click::network::Reply> reply;
    QString downloadUrl;
    QString appId;
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

void click::DownloadManager::startDownload(const QString& downloadUrl, const QString& appId)
{
    impl->appId = appId;

    // NOTE: using SIGNAL/SLOT macros here because new-style
    // connections are flaky on ARM.
    QObject::connect(this, SIGNAL(clickTokenFetched(QString)),
                     this, SLOT(handleClickTokenFetched(QString)),
                     Qt::UniqueConnection);
    QObject::connect(this, SIGNAL(clickTokenFetchError(QString)),
                     this, SLOT(handleClickTokenFetchError(QString)),
                     Qt::UniqueConnection);
    fetchClickToken(downloadUrl);
}

void click::DownloadManager::handleClickTokenFetched(const QString& clickToken)
{
    QVariantMap metadata;
    metadata[DOWNLOAD_COMMAND_KEY] = DOWNLOAD_CMDLINE;
    metadata[DOWNLOAD_APP_ID_KEY] = impl->appId;

    QMap<QString, QString> headers;
    headers[CLICK_TOKEN_HEADER()] = clickToken;

    udm::DownloadStruct downloadStruct(impl->downloadUrl,
                                       DOWNLOAD_MANAGER_DO_NOT_HASH,
                                       DOWNLOAD_MANAGER_IGNORE_HASH_ALGORITHM,
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

void click::DownloadManager::fetchClickToken(const QString& downloadUrl)
{
    impl->downloadUrl = downloadUrl;
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
    emit clickTokenFetchError(QString("No creds found"));
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
    qDebug() << "error in network request for click token: " << error << impl->reply->errorString();
    impl->reply.reset();
    emit clickTokenFetchError(QString("Network Error"));
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

void click::Downloader::get_download_progress(std::string /*package_name*/, const std::function<void (std::string)>& /*callback*/)
{
    // TODO, unimplemented. see https://bugs.launchpad.net/ubuntu-download-manager/+bug/1277814
}

void click::Downloader::startDownload(std::string url, std::string package_name, const std::function<void (std::pair<std::string, boost::optional<std::string> >)>& callback)
{
    qt::core::world::enter_with_task([this, callback, url, package_name] (qt::core::world::Environment& /*env*/)
    {
        auto& dm = downloadManagerInstance(networkAccessManager);

        QObject::connect(&dm, &click::DownloadManager::downloadStarted,
                         [callback](QString downloadId)
                         {
                             std::cout << "Download started: " << downloadId.toStdString() << std::endl;
                             auto ret = std::pair<std::string, boost::optional<std::string> >(downloadId.toUtf8().data(), boost::optional<std::string>());
                             callback(ret);
                         });
        QObject::connect(&dm, &click::DownloadManager::downloadError,
                         [callback](QString errorMessage)
                         {
                             qDebug() << "Error creating download:" << errorMessage;
                             auto ret = std::pair<std::string, boost::optional<std::string> >(std::string(), boost::optional<std::string>(errorMessage.toStdString()));
                             callback(ret);
                         });
        std::cout << "About to call start download" << std::endl;
        dm.startDownload(QString::fromStdString(url),
                          QString::fromStdString(package_name));
    });
}
