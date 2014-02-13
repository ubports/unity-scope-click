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

#include <unity/UnityExceptions.h>
#include <unity/util/IniParser.h>

#include "interface.h"
#include "key_file_locator.h"

namespace click {

const std::unordered_set<std::string>& nonClickDesktopFiles()
{
    static std::unordered_set<std::string> set =
    {
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
    };

    return set;
}

static const std::string DESKTOP_FILE_GROUP("Desktop Entry");
static const std::string DESKTOP_FILE_KEY_NAME("Name");
static const std::string DESKTOP_FILE_KEY_ICON("Icon");
static const std::string DESKTOP_FILE_KEY_APP_ID("X-Ubuntu-Application-ID");
static const std::string DESKTOP_FILE_UBUNTU_TOUCH("X-Ubuntu-Touch");
static const std::string DESKTOP_FILE_COMMENT("Comment");
static const std::string DESKTOP_FILE_SCREENSHOT("X-Screenshot");

Interface::Interface(const QSharedPointer<click::KeyFileLocator>& keyFileLocator)
    : keyFileLocator(keyFileLocator)
{
}

Interface::~Interface()
{
}

/* find_installed_apps()
 *
 * Find all of the isntalled apps matching @search_query in a timeout.
 * The list of found apps will be emitted in the ::installed_apps_found
 * signal on this object.
 */
std::list<click::Application> Interface::find_installed_apps(const QString& search_query)
{
    std::list<Application> result;

    auto enumerator = [&result, search_query](const unity::util::IniParser& keyFile, const std::string& filename)
    {
        if (keyFile.has_key(DESKTOP_FILE_GROUP, DESKTOP_FILE_KEY_APP_ID)
            || Interface::is_non_click_app(QString::fromStdString(filename))) {
            QString name = keyFile.get_string(DESKTOP_FILE_GROUP,
                                              DESKTOP_FILE_KEY_NAME).c_str();
            if (search_query.isEmpty() ||
                (name != NULL && name.contains(search_query,
                                               Qt::CaseInsensitive))) {
                Application app;
                QString app_url = "application:///" + QString::fromStdString(filename);
                app.url = app_url.toUtf8().data();
                app.title = name.toUtf8().data();
                app.icon_url = keyFile.get_string(DESKTOP_FILE_GROUP,
                                                  DESKTOP_FILE_KEY_ICON);
                if (keyFile.has_key(DESKTOP_FILE_GROUP, DESKTOP_FILE_UBUNTU_TOUCH)) {
                    if (keyFile.has_key(DESKTOP_FILE_GROUP, DESKTOP_FILE_COMMENT)) {
                        app.description = keyFile.get_string(DESKTOP_FILE_GROUP,
                                                             DESKTOP_FILE_COMMENT);
                    }
                    if (keyFile.has_key(DESKTOP_FILE_GROUP, DESKTOP_FILE_SCREENSHOT)) {
                        app.main_screenshot = keyFile.get_string(DESKTOP_FILE_GROUP,
                                                                 DESKTOP_FILE_SCREENSHOT);
                    }
                } else if (keyFile.has_key(DESKTOP_FILE_GROUP, DESKTOP_FILE_KEY_APP_ID)) {
                    QString app_id = QString::fromStdString(keyFile.get_string(
                                                            DESKTOP_FILE_GROUP,
                                                            DESKTOP_FILE_KEY_APP_ID));
                    QStringList id = app_id.split("_", QString::SkipEmptyParts);
                    app.name = id[0].toUtf8().data();
                }
                result.push_front(app);
            }
        }
    };

    keyFileLocator->enumerateKeyFilesForInstalledApplications(enumerator);

    return std::move(result);
}

/* is_non_click_app()
 *
 * Tests that @filename is one of the special-cased filenames for apps
 * which are not packaged as clicks, but required on Ubuntu Touch.
 */
bool Interface::is_non_click_app(const QString& filename)
{
    return click::nonClickDesktopFiles().count(filename.toUtf8().data()) > 0;
}

/* find_apps_in_dir()
 *
 * Finds all the apps in the specified @dir_path, which also match
 * the @search_query string, and appends them to the @result_list.
 */
void Interface::find_apps_in_dir(const QString& dir_path,
                                 const QString& search_query,
                                 std::list<Application>& result_list)
{
    QDir dir(dir_path, "*.desktop",
             QDir::Unsorted, QDir::Readable | QDir::Files);
    QStringList entries = dir.entryList();
    for (int i = 0; i < entries.size(); ++i) {
        QString filename = entries.at(i);
        QString full_path = dir.absoluteFilePath(filename);
        try {

            unity::util::IniParser keyfile(full_path.toUtf8().data());
            if (keyfile.has_key(DESKTOP_FILE_GROUP, DESKTOP_FILE_KEY_APP_ID)
                || Interface::is_non_click_app(filename)) {
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
                    if (keyfile.has_key(DESKTOP_FILE_GROUP, DESKTOP_FILE_UBUNTU_TOUCH)) {
                        app.description = keyfile.get_string(DESKTOP_FILE_GROUP,
                                                             DESKTOP_FILE_COMMENT);
                        app.main_screenshot = keyfile.get_string(DESKTOP_FILE_GROUP,
                                                                 DESKTOP_FILE_SCREENSHOT);
                    } else if (keyfile.has_key(DESKTOP_FILE_GROUP, DESKTOP_FILE_KEY_APP_ID)) {
                        QString app_id = QString::fromStdString(keyfile.get_string(
                                                                DESKTOP_FILE_GROUP,
                                                                DESKTOP_FILE_KEY_APP_ID));
                        QStringList id = app_id.split("_", QString::SkipEmptyParts);
                        app.name = id[0].toUtf8().data();
                    }
                    qDebug() << "Found application:" << filename;
                    result_list.push_front(app);
                }
            }
        } catch (unity::FileException file_exp) {
            qCritical() << "Error reading file:" << full_path << "skipping.";
        }
    }
}

} // namespace click
