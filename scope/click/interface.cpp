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

#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QTimer>

#include <list>

#include <unity/util/IniParser.h>

#include "interface.h"

namespace click {

#define DESKTOP_FILE_GROUP "Desktop Entry"
#define DESKTOP_FILE_KEY_NAME "Name"
#define DESKTOP_FILE_KEY_ICON "Icon"
#define DESKTOP_FILE_KEY_APP_ID "X-Ubuntu-Application-ID"

#define NON_CLICK_PATH "/usr/share/applications"

Interface::Interface(QObject *parent)
    : QObject(parent)
{
}

void Interface::find_installed_apps(const QString& search_query)
{
    qDebug() << "Finding apps matching query:" << search_query;
    query_string = search_query;
    find_apps_timer.setSingleShot(true);
    QObject::connect(&find_apps_timer, &QTimer::timeout, [&]() {
            find_installed_apps_real();
        } );
    find_apps_timer.start(0);
}

static bool is_non_click_app(const QString& filename)
{
    // List of the desktop files that are not yet click packages
    const std::list<std::string> NON_CLICK_DESKTOPS = {{
            "address-book-app.desktop",
            "camera-app.desktop",
            "click-update-manager.desktop",
            "dialer-app.desktop",
            "friends-app.desktop",
            "gallery-app.desktop",
            "mediaplayer-app.desktop",
            "messaging-app.desktop",
            "music-app.desktop",
            "ubuntu-filemanager-app.desktop",
            "ubuntu-system-settings.desktop",
            "webbrowser-app.desktop",
        }};
    if (std::any_of(NON_CLICK_DESKTOPS.cbegin(), NON_CLICK_DESKTOPS.cend(), 
                    [&filename](std::string s){ return s == filename.toUtf8().data();} )) {
        return true;
    }
    return false;
}

static void find_apps_in_dir(const QString& dir_path,
                             const QString& search_query,
                             std::list<Application> result_list)
{
    QDir dir(dir_path, "*.desktop",
             QDir::Unsorted, QDir::Readable | QDir::Files);
    QStringList entries = dir.entryList();
    for (int i = 0; i < entries.size(); ++i) {
        QString filename = entries.at(i);
        unity::util::IniParser keyfile(dir.absoluteFilePath(filename).toUtf8().data());
        if (keyfile.has_key(DESKTOP_FILE_GROUP, DESKTOP_FILE_KEY_APP_ID)
            || is_non_click_app(filename)) {
            QString name = keyfile.get_string(DESKTOP_FILE_GROUP,
                                              DESKTOP_FILE_KEY_NAME).c_str();
            if (search_query.isEmpty() ||
                (name != NULL && name.contains(search_query,
                                               Qt::CaseInsensitive))) {
                Application app;
                QString app_url = "application:///" + filename;
                app.url = app_url.toUtf8().data();
                app.title = name.toUtf8().data();
                app.icon_url = keyfile.get_string(DESKTOP_FILE_GROUP,
                                                  DESKTOP_FILE_KEY_ICON);
                qDebug() << "Found application:" << name;
                result_list.push_front(app);
            }
        }
    }
}

void Interface::find_installed_apps_real()
{
    std::list<Application> result;
    // Get the non-click apps
    qDebug() << "Searching for installed non-click apps.";
    find_apps_in_dir(NON_CLICK_PATH, query_string, result);

    // Get the installed click apps
    QString click_path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/applications";
    qDebug() << "Searching for installed click apps.";
    find_apps_in_dir(click_path, query_string, result);

    emit installed_apps_found(result);
}

} // namespace click
