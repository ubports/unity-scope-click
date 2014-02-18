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

#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include <QTimer>
#include <QTextStream>

#include <iostream>

#include <boost/optional.hpp>

#include <download_manager_tool.h>

DownloadManagerTool::DownloadManagerTool(QObject *parent):
     QObject(parent)
{
    _dm = new click::DownloadManager(QSharedPointer<click::network::AccessManager>(new click::network::AccessManager()),
                                     QSharedPointer<click::CredentialsService>(new click::CredentialsService()),
                                     QSharedPointer<Ubuntu::DownloadManager::Manager>(
                                         Ubuntu::DownloadManager::Manager::createSessionManager()));
}

void DownloadManagerTool::handleResponse(QString response)
{
    QTextStream(stdout) << "DONE: response is " << response << "\n";
    emit finished();
}

void DownloadManagerTool::fetchClickToken(QString url)
{
    QObject::connect(_dm, &click::DownloadManager::clickTokenFetched,
                     this, &DownloadManagerTool::handleResponse);
    QObject::connect(_dm, &click::DownloadManager::clickTokenFetchError,
                     this, &DownloadManagerTool::handleResponse);
    _dm->fetchClickToken(url);
}

void DownloadManagerTool::startDownload(QString url, QString appId)
{
    QObject::connect(_dm, &click::DownloadManager::downloadStarted,
                     this, &DownloadManagerTool::handleResponse);
    QObject::connect(_dm, &click::DownloadManager::downloadError,
                     this, &DownloadManagerTool::handleResponse);
    _dm->startDownload(url, appId);
}

int main(int argc, char *argv[])
{

    QCoreApplication a(argc, argv);
    DownloadManagerTool tool;
    click::Downloader downloader(QSharedPointer<click::network::AccessManager>(new click::network::AccessManager()));
    QTimer timer;
    timer.setSingleShot(true);

    QObject::connect(&tool, SIGNAL(finished()), &a, SLOT(quit()));

    if (argc == 2) {

        QObject::connect(&timer, &QTimer::timeout, [&]() {
                tool.fetchClickToken(QString(argv[1]));
            } );

    } else if (argc == 3) {

        QObject::connect(&timer, &QTimer::timeout, [&]() {
                downloader.startDownload(std::string(argv[1]), std::string(argv[2]),
                                         [&a] (std::pair<std::string, boost::optional<std::string> > arg){
                                             auto download_id = arg.first;
                                             auto error = arg.second;
                                             if (!error) {
                                                 std::cout << " Success, got download ID:" << download_id << std::endl;
                                             } else {
                                                 std::cout << " Error:" << error << std::endl;
                                             }
                                             a.quit();
                                         });
                
            } );
        
    } else {
        QTextStream(stderr) << "Usages:\n"
                            << "download_manager_tool https://public.apps.ubuntu.com/download/<<rest of click package dl url>>\n" 
                            << "\t - when run with a valid U1 credential in the system, should print the click token to stdout.\n"
                            << "download_manager_tool url app_id\n"
                            << "\t - with a valid credential, should begin a download.\n";
        
        return 1;
    }

    timer.start(0);
        
    qInstallMessageHandler(0);
    return a.exec();
}

