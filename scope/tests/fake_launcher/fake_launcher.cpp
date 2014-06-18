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
#include <ubuntu/download_manager/downloads_list.h>
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

void FakeLauncher::startInstallation(QString title, QString icon_url, QString package_name)
{
    qDebug() << "startInstallation" << title << icon_url << package_name;
    installations.insert(package_name, new FakeIcon(title, icon_url, this));
    manager->getAllDownloadsWithMetadata("package_name", package_name);
}

void FakeLauncher::handleDownloadsFound(const QString& key, const QString& value, DownloadsList *download_list)
{
    Q_UNUSED(key)
    Q_UNUSED(value)
    foreach (const QSharedPointer<Download> download, download_list->downloads()) {
        QString package_name{download->metadata()["package_name"].toString()};
        qDebug() << "download found for" << package_name;
        installations[package_name]->downloadFound(*download);
    }
}

void FakeLauncher::completeInstallation(QString package_name, QString app_id)
{
    qDebug() << "completeInstallation" << package_name << app_id;
    installations[package_name]->complete(app_id);
}

void FakeIcon::downloadFound(Download& download)
{
    connect(&download, SIGNAL(progress(qulonglong,qulonglong)), this, SLOT(handleProgress(qulonglong,qulonglong)));
    connect(&download, SIGNAL(error(Error*)), this, SLOT(handleDownloadError(Error*)));
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
    new FakeLauncherAdaptor(&launcher);
    qDebug() << "starting";
    auto bus = QDBusConnection::sessionBus();
    bus.registerObject(LAUNCHER_OBJECT_PATH, &launcher);
    bus.registerService(LAUNCHER_BUSNAME);
    return app.exec();
}

