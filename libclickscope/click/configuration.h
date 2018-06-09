/*
 * Copyright (C) 2014-2015 Canonical Ltd.
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

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <map>
#include <string>
#include <vector>

namespace click
{

class Configuration
{
public:
    constexpr static const char* FRAMEWORKS_FOLDER {"/usr/share/click/frameworks/"};
    constexpr static const char* FRAMEWORKS_PATTERN {"*.framework"};
    constexpr static const int FRAMEWORKS_EXTENSION_LENGTH = 10; // strlen(".framework")
    constexpr static const char* ARCH_ENVVAR {"U1_SEARCH_ARCH"};
    constexpr static const char* LANGUAGE_ENVVAR {"LANGUAGE"};
    static const std::vector<const char*> FULL_LANG_CODES;

    virtual std::vector<std::string> get_available_frameworks();
    virtual std::string get_architecture();

    virtual std::string get_language_base();
    virtual std::string get_language();
    virtual std::string get_accept_languages();
    static bool is_full_lang_code(const std::string& language);

    virtual std::string get_device_id();

    constexpr static const char* COREAPPS_SCHEMA {"com.canonical.Unity.ClickScope"};
    constexpr static const char* COREAPPS_KEY {"coreApps"};
    constexpr static const char* IGNORED_KEY {"ignoredApps"};

    virtual const std::vector<std::string> get_core_apps() const;
    virtual const std::vector<std::string> get_ignored_apps() const;
    virtual ~Configuration() {}
protected:
    virtual std::vector<std::string> list_folder(const std::string &folder, const std::string &pattern);
    virtual std::string architectureFromDpkg();
    virtual std::string deviceIdFromWhoopsie();
    virtual const std::vector<std::string> get_dconf_strings(const std::string& schema, const std::string& key) const;
    static const std::vector<std::string>& get_default_core_apps() {
        static std::vector<std::string> default_apps {
            "dialer-app",
            "messaging-app",
            "address-book-app",
            "com.ubuntu.camera_camera",
            "webbrowser-app",
            "com.ubuntu.clock_clock"
        };
        return default_apps;
    }
};

} // namespace click

#endif // CONFIGURATION_H
