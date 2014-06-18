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

#ifndef _FAKE_LAUNCHER_H_
#define _FAKE_LAUNCHER_H_

#include <QObject>
#include <QDBusObjectPath>
#include <QDBusAbstractAdaptor>

#include <click/dbus_constants.h>
#include <ubuntu/download_manager/manager.h>

class FakeIcon : public QObject {
    Q_OBJECT

public:
    explicit FakeIcon(QString title, QString icon_url, QObject *parent=0)
        : QObject(parent), title(title), icon_url(icon_url)
    {
    }
    void downloadFound(Download& download);
    void complete(QString app_id);

private slots:
    void handleProgress(qulonglong transferred, qulonglong total);
    void handleDownloadError(Error* error);

private:
    Download* download;
    QString title;
    QString icon_url;
    void failure(QString message);
};

class FakeLauncher : public QObject {
    Q_OBJECT

public:
    explicit FakeLauncher(QObject *parent=0) : QObject(parent)
    {
        manager = Ubuntu::DownloadManager::Manager::createSessionManager();
        connect(manager, SIGNAL(downloadsWithMetadataFound(QString,QString,DownloadsList*)),
                this, SLOT(handleDownloadsFound(QString,QString,DownloadsList*)));
    }
                                                             
public slots:
    void startInstallation(QString title, QString icon_url, QString package_name);
    void completeInstallation(QString package_name, QString app_id);

private slots:
    void handleDownloadsFound(const QString& key, const QString& value, DownloadsList* downloads);

signals:

private:
    Ubuntu::DownloadManager::Manager* manager;
    QMap<QString, FakeIcon*> installations;
};


class FakeLauncherAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", LAUNCHER_INTERFACE)

private:
    FakeLauncher *_launcher;

public:
    FakeLauncherAdaptor(FakeLauncher *launcher) : QDBusAbstractAdaptor(launcher), _launcher(launcher)
    {
    }

public slots:
    Q_NOREPLY void startInstallation(QString title, QString icon_url, QString package_name)
    {
        _launcher->startInstallation(title, icon_url, package_name);
    }

    Q_NOREPLY void completeInstallation(QString package_name, QString app_id)
    {
        _launcher->completeInstallation(package_name, app_id);
    }
};


#endif /* _FAKE_LAUNCHER_H_ */


