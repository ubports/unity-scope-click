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

namespace click {

static const std::string DESKTOP_FILE_GROUP("Desktop Entry");
static const std::string DESKTOP_FILE_KEY_NAME("Name");
static const std::string DESKTOP_FILE_KEY_ICON("Icon");
static const std::string DESKTOP_FILE_KEY_APP_ID("X-Ubuntu-Application-ID");

static const QString NON_CLICK_PATH("/usr/share/applications");

Interface::Interface(QObject *parent)
    : QObject(parent)
{
}

/* find_installed_apps()
 *
 * Find all of the isntalled apps matching @search_query in a timeout.
 * The list of found apps will be emitted in the ::installed_apps_found
 * signal on this object.
 */
void Interface::find_installed_apps(const QString& search_query)
{
    qDebug() << "Finding apps matching query:" << search_query;
    query_string = search_query;
    QTimer::singleShot(0, this, SLOT(find_installed_apps_real()));
}

/* is_non_click_app()
 *
 * Tests that @filename is one of the special-cased filenames for apps
 * which are not packaged as clicks, but required on Ubuntu Touch.
 */
bool Interface::is_non_click_app(const QString& filename)
{
    if (std::any_of(NON_CLICK_DESKTOPS.cbegin(), NON_CLICK_DESKTOPS.cend(), 
                    [&filename](std::string s){ return s == filename.toUtf8().data();} )) {
        return true;
    }
    return false;
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
                    qDebug() << "Found application:" << filename;
                    result_list.push_front(app);
                }
            }
        } catch (unity::FileException file_exp) {
            qCritical() << "Error reading file:" << full_path << "skipping.";
        }
    }
}

/* find_installed_apps_real()
 *
 * The slot used for finding the installed click and special non-click apps,
 * on the system, from within an asynchronous timeout on the event loop.
 */
void Interface::find_installed_apps_real()
{
    std::list<Application> result;
    // Get the non-click apps
    qDebug() << "Searching for installed non-click apps.";
    Interface::find_apps_in_dir(NON_CLICK_PATH, query_string, result);

    // Get the installed click apps
    QString click_path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/applications";
    qDebug() << "Searching for installed click apps.";
    Interface::find_apps_in_dir(click_path, query_string, result);

    emit installed_apps_found(result);
}

} // namespace click
