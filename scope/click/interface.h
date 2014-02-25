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

#ifndef CLICK_INTERFACE_H
#define CLICK_INTERFACE_H

#include <QObject>
#include <QStringList>

#include <vector>
#include <unordered_set>

#include "application.h"

namespace click
{

class KeyFileLocator;

// Hash map of desktop files that are not yet click packages
const std::unordered_set<std::string>& nonClickDesktopFiles();

class Interface
{
public:
    Interface(const QSharedPointer<KeyFileLocator>& keyFileLocator);
    virtual ~Interface();

    virtual std::vector<Application> find_installed_apps(const QString& search_query);

    static bool is_non_click_app(const QString& filename);
    static void find_apps_in_dir(const QString& dir_path,
                                 const QString& search_query,
                                 std::vector<Application>& result_list);

    static bool is_icon_identifier(const std::string &icon_id);
    static std::string add_theme_scheme(const std::string &filename);
private:
    QSharedPointer<KeyFileLocator> keyFileLocator;
};

class FrameworkLocator
{
public:
    constexpr static const char* FRAMEWORKS_FOLDER {"/usr/share/click/frameworks/"};
    constexpr static const char* FRAMEWORKS_PATTERN {"*.framework"};
    constexpr static const int FRAMEWORKS_EXTENSION_LENGTH = 10; // size(".framework")

    virtual std::vector<std::string> get_available_frameworks();
protected:
    virtual std::vector<std::string> list_folder(const std::string &folder, const std::string &pattern);
};

} // namespace click

#endif // CLICK_INTERFACE_H
