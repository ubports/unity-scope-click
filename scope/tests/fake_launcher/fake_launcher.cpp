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

#include <ubuntu/download_manager/download.h>
#include <ubuntu/download_manager/error.h>

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDebug>
#include <QString>
#include <QTimer>
#include <QTextStream>

#include <iostream>

#include <boost/optional.hpp>

#include <fake_launcher.h>

void FakeLauncher::startInstallation(QString title, QString icon_url, QString download_id)
{
    auto download = manager->getDownloadForId(download_id);
    installations.insert(download_id, new FakeIcon(download, title, icon_url, this));
    qDebug() << "startInstallation" << title << icon_url << download_id;
}

void FakeLauncher::completeInstallation(QString download_id, QString app_id)
{
    qDebug() << "completeInstallation" << download_id << app_id;
    installations[download_id]->complete(app_id);
}

void FakeIcon::setup()
{
    connect(download, SIGNAL(progress(qulonglong,qulonglong)), this, SLOT(progress(qulonglong,qulonglong)));
    connect(download, SIGNAL(error(Error*)), this, SLOT(downloadError(Error*)));
    qDebug() << title << "starting installation";
    // TODO: add icon to the launcher
}

void FakeIcon::handleProgress(qulonglong transferred, qulonglong total)
{
    qDebug() << title << "download progress" << double(transferred)/total*80;
    // TODO: update progress bar
}

void FakeIcon::handleDownloadError(Error *error)
{
    failure(error->errorString());
}

void FakeIcon::failure(QString message)
{
    qDebug() << title << "installation failed" << message;
    // TODO: remove icon from the launcher
}

void FakeIcon::complete(QString app_id)
{
    if (app_id.isEmpty()) {
        failure("Failed to install");
    } else {
        qDebug() << title << "installation completed" << app_id;
        // TODO: update icon with proper app_id
    }
}

int main(int argc, char *argv[])
{

    QCoreApplication app(argc, argv);
    FakeLauncher launcher;
    FakeLauncherAdaptor adaptor(&launcher);

    auto bus = QDBusConnection::connectToBus(QDBusConnection::SessionBus, LAUNCHER_BUSNAME);
    bus.registerObject(LAUNCHER_OBJECT_PATH, &launcher);

    return app.exec();
}

