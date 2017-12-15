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
#include <unity/util/IniParser.h>

#include <vector>
#include <unordered_set>

#include "application.h"
#include "package.h"

// The dbus-send command to refresh the search results in the dash.
static const QString REFRESH_SCOPE_COMMAND = QStringLiteral("dbus-send /com/canonical/unity/scopes com.canonical.unity.scopes.InvalidateResults string:%1");
static const QString APPS_SCOPE_ID = QStringLiteral("clickscope");

namespace click
{

class KeyFileLocator;
class DepartmentsDb;

// Hash map of desktop files that are not yet click packages
const std::unordered_set<std::string>& nonClickDesktopFiles();

struct Manifest
{
    Manifest() = default;
    Manifest(std::string name, std::string version, std::string first_app_name) :
        name(name), version(version), first_app_name(first_app_name)
    {
    }
    virtual ~Manifest() = default;

    std::string name;
    std::string version;
    std::string first_app_name;
    std::string first_scope_id;
    bool removable = false;
    bool has_any_apps() const {
        return !first_app_name.empty();
    }
    bool has_any_scopes() const {
        return !first_scope_id.empty();
    }
};

enum class InterfaceError {NoError, CallError, ParseError};
typedef std::list<Manifest> ManifestList;

ManifestList manifest_list_from_json(const std::string& json);
Manifest manifest_from_json(const std::string& json);
PackageSet package_names_from_stdout(const std::string& stdout_data);

class Interface
{
public:
    Interface(const QSharedPointer<KeyFileLocator>& keyFileLocator);
    Interface() = default;
    virtual ~Interface();

    virtual std::string get_translated_string(const unity::util::IniParser& keyFile,
                                              const std::string& group,
                                              const std::string& key,
                                              const std::string& domain);
    virtual Application load_app_from_desktop(const unity::util::IniParser& keyFile,
                                              const std::string& filename);
    static std::vector<Application> sort_apps(const std::vector<Application>& apps);
    virtual std::vector<Application> find_installed_apps(const std::string& search_query,
            const std::vector<std::string>& ignored_apps = std::vector<std::string>{},
            const std::string& current_department = "",
            const std::shared_ptr<click::DepartmentsDb>& depts_db = nullptr);

    static bool is_non_click_app(const QString& filename);

    static bool is_icon_identifier(const std::string &icon_id);
    static std::string add_theme_scheme(const std::string &filename);
    virtual void get_manifests(std::function<void(ManifestList, InterfaceError)> callback);
    virtual void get_installed_packages(std::function<void(PackageSet, InterfaceError)> callback);
    virtual void get_manifest_for_app(const std::string &app_id, std::function<void(Manifest, InterfaceError)> callback);
    constexpr static const char* ENV_SHOW_DESKTOP_APPS {"CLICK_SCOPE_SHOW_DESKTOP_APPS"};
    virtual bool is_visible_app(const unity::util::IniParser& keyFile);
    virtual bool show_desktop_apps();

    virtual void run_process(const std::string& command,
                             std::function<void(int code,
                                                const std::string& stdout_data,
                                                const std::string& stderr_data)> callback);
private:
    QSharedPointer<KeyFileLocator> keyFileLocator;
};

} // namespace click

#endif // CLICK_INTERFACE_H
