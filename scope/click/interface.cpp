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
#include <sys/stat.h>
#include <map>

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
std::vector<click::Application> Interface::find_installed_apps(const QString& search_query)
{
    std::vector<Application> result;

    std::map<std::string, time_t> installTimes;
    auto enumerator = [&result, &installTimes, search_query](const unity::util::IniParser& keyFile, const std::string& filename)
    {
        if (keyFile.has_key(DESKTOP_FILE_GROUP, DESKTOP_FILE_KEY_APP_ID)
            || Interface::is_non_click_app(QString::fromStdString(filename))) {
            QString name = keyFile.get_string(DESKTOP_FILE_GROUP,
                                              DESKTOP_FILE_KEY_NAME).c_str();
            if (search_query.isEmpty() ||
                (name != NULL && name.contains(search_query,
                                               Qt::CaseInsensitive))) {
                Application app;
                struct stat times;
                installTimes[filename] = stat(filename.c_str(), &times) == 0 ? times.st_mtime : 0;
                QString app_url = "application:///" + QString::fromStdString(filename);
                app.url = app_url.toUtf8().data();
                app.title = name.toUtf8().data();
                app.icon_url = Interface::add_theme_scheme(
                            keyFile.get_string(DESKTOP_FILE_GROUP,
                                               DESKTOP_FILE_KEY_ICON));
                if (keyFile.has_key(DESKTOP_FILE_GROUP, DESKTOP_FILE_KEY_APP_ID)) {
                    QString app_id = QString::fromStdString(keyFile.get_string(
                                                            DESKTOP_FILE_GROUP,
                                                            DESKTOP_FILE_KEY_APP_ID));
                    QStringList id = app_id.split("_", QString::SkipEmptyParts);
                    app.name = id[0].toUtf8().data();
                }
                result.push_back(app);
            }
        }
    };

    keyFileLocator->enumerateKeyFilesForInstalledApplications(enumerator);
    // Sort applications so that newest come first.
    std::sort(result.begin(), result.end(), [&installTimes](const Application& a, const Application& b)
            {return installTimes[a.name] > installTimes[b.name];});
    return result;
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

/*
 * is_icon_identifier()
 *
 * Checks if @filename has no / in it
 */
bool Interface::is_icon_identifier(const std::string &icon_id)
{
    return icon_id.find("/") == std::string::npos;
}

/*
 * add_theme_scheme()
 *
 * Adds the theme prefix if the filename is not an icon identifier
 */
std::string Interface::add_theme_scheme(const std::string& icon_id)
{
    if (is_icon_identifier(icon_id)) {
        return "image://theme/" + icon_id;
    }
    return icon_id;
}

/* find_apps_in_dir()
 *
 * Finds all the apps in the specified @dir_path, which also match
 * the @search_query string, and appends them to the @result_list.
 */
void Interface::find_apps_in_dir(const QString& dir_path,
                                 const QString& search_query,
                                 std::vector<Application>& result_list)
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
                    app.icon_url = Interface::add_theme_scheme(
                                keyfile.get_string(DESKTOP_FILE_GROUP,
                                                   DESKTOP_FILE_KEY_ICON));
                    qDebug() << "Found application:" << filename;
                    result_list.push_back(app);
                }
            }
        } catch (unity::FileException file_exp) {
            qCritical() << "Error reading file:" << full_path << "skipping.";
        }
    }
}

} // namespace click
