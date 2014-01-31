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

#include <download_manager_tool.h>

DownloadManagerTool::DownloadManagerTool(QObject *parent):
     QObject(parent)
{
    QObject::connect(&_dm, &click::DownloadManager::clickTokenFetched,
                     this, &DownloadManagerTool::handleFetchResponse);
    QObject::connect(&_dm, &click::DownloadManager::clickTokenFetchError,
                     this, &DownloadManagerTool::handleFetchResponse);
}

void DownloadManagerTool::handleFetchResponse(QString response)
{
    QTextStream(stdout) << "DONE: response is " << response << "\n";
    emit finished();
}

void DownloadManagerTool::fetchClickToken()
{
    _dm.fetchClickToken(_url);
}

int main(int argc, char *argv[])
{

    QCoreApplication a(argc, argv);
    DownloadManagerTool tool(&a);

    if (argc != 2) {
        QTextStream(stderr) << "Usage: download_manager_tool https://public.apps.ubuntu.com/download/<<rest of click package dl url>>" 
                            << "\n\t - when run with a valid U1 credential in the system, should print the click token to stdout.\n";
        return 1;
    }
        

    tool.setClickTokenURL(QString(argv[1]));

    QObject::connect(&tool, SIGNAL(finished()), &a, SLOT(quit()));

    QTimer::singleShot(0, &tool, SLOT(fetchClickToken()));

    return a.exec();
}
