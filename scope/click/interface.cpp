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
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>

#include <list>
#include <sys/stat.h>
#include <map>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/exceptions.hpp>
#include <boost/foreach.hpp>

#include <unity/UnityExceptions.h>
#include <unity/util/IniParser.h>
#include <sstream>

#include "interface.h"
#include <click/key_file_locator.h>

#include <click/click-i18n.h>

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
static const std::string DESKTOP_FILE_KEY_DOMAIN("X-Ubuntu-Gettext-Domain");
static const std::string DESKTOP_FILE_UBUNTU_TOUCH("X-Ubuntu-Touch");
static const std::string DESKTOP_FILE_COMMENT("Comment");
static const std::string DESKTOP_FILE_SCREENSHOT("X-Screenshot");
static const std::string DESKTOP_FILE_NODISPLAY("NoDisplay");
static const std::string DESKTOP_FILE_ONLYSHOWIN("OnlyShowIn");
static const std::string ONLYSHOWIN_UNITY("Unity");

Interface::Interface(const QSharedPointer<click::KeyFileLocator>& keyFileLocator)
    : keyFileLocator(keyFileLocator)
{
}

Interface::~Interface()
{
}

bool Interface::show_desktop_apps()
{
    return getenv(ENV_SHOW_DESKTOP_APPS) != nullptr;
}

bool Interface::is_visible_app(const unity::util::IniParser &keyFile)
{
    if (keyFile.has_key(DESKTOP_FILE_GROUP, DESKTOP_FILE_NODISPLAY)) {
        auto val = keyFile.get_string(DESKTOP_FILE_GROUP, DESKTOP_FILE_NODISPLAY);
        if (val == std::string("true")) {
            return false;
        }
    }

    if (keyFile.has_key(DESKTOP_FILE_GROUP, DESKTOP_FILE_ONLYSHOWIN)) {
        auto value = keyFile.get_string(DESKTOP_FILE_GROUP, DESKTOP_FILE_ONLYSHOWIN);
        std::stringstream ss(value);
        std::string item;

        while (std::getline(ss, item, ';')) {
            if (item == ONLYSHOWIN_UNITY) {
                return true;
            }
        }
        return false;
    }

    return true;
}

std::string Interface::get_translated_string(const unity::util::IniParser& keyFile,
                                             const std::string& group,
                                             const std::string& key,
                                             const std::string& domain)
{
    std::string language = Configuration().get_language();
    if (!domain.empty()) {
        return dgettext(domain.c_str(),
                        keyFile.get_string(group, key).c_str());
    } else {
        return keyFile.get_locale_string(group, key, language);
    }
}

click::Application Interface::load_app_from_desktop(const unity::util::IniParser& keyFile,
                                                    const std::string& filename)
{
    Application app;
    std::string domain;
    if (keyFile.has_key(DESKTOP_FILE_GROUP, DESKTOP_FILE_KEY_DOMAIN)) {
        domain = keyFile.get_string(DESKTOP_FILE_GROUP,
                                    DESKTOP_FILE_KEY_DOMAIN);
    }
    app.title = get_translated_string(keyFile,
                                      DESKTOP_FILE_GROUP,
                                      DESKTOP_FILE_KEY_NAME,
                                      domain);
    struct stat times;
    app.installed_time = stat(filename.c_str(), &times) == 0 ? times.st_mtime : 0;
    app.url = "application:///" + filename;
    if (keyFile.has_key(DESKTOP_FILE_GROUP, DESKTOP_FILE_KEY_ICON)) {
        app.icon_url = add_theme_scheme(keyFile.get_string(DESKTOP_FILE_GROUP,
                                                           DESKTOP_FILE_KEY_ICON));
    }
    if (keyFile.has_key(DESKTOP_FILE_GROUP, DESKTOP_FILE_KEY_APP_ID)) {
        QString app_id = QString::fromStdString(keyFile.get_string(
                DESKTOP_FILE_GROUP,
                DESKTOP_FILE_KEY_APP_ID));
        QStringList id = app_id.split("_", QString::SkipEmptyParts);
        app.name = id[0].toUtf8().data();
        app.version = id[2].toUtf8().data();
    } else {
        if (keyFile.has_key(DESKTOP_FILE_GROUP, DESKTOP_FILE_COMMENT)) {
            app.description = get_translated_string(keyFile,
                                                    DESKTOP_FILE_GROUP,
                                                    DESKTOP_FILE_COMMENT,
                                                    domain);
        }
        if (keyFile.has_key(DESKTOP_FILE_GROUP, DESKTOP_FILE_SCREENSHOT)) {
            app.main_screenshot = keyFile.get_string(DESKTOP_FILE_GROUP,
                                                     DESKTOP_FILE_SCREENSHOT);
        }
    }
    return app;
}

/* find_installed_apps()
 *
 * Find all of the installed apps matching @search_query in a timeout.
 */
std::vector<click::Application> Interface::find_installed_apps(const std::string& search_query)
{
    std::vector<Application> result;

    bool include_desktop_results = show_desktop_apps();

    auto enumerator = [&result, this, search_query, include_desktop_results]
            (const unity::util::IniParser& keyFile, const std::string& filename)
    {
        if (keyFile.has_group(DESKTOP_FILE_GROUP) == false) {
            qWarning() << "Broken desktop file:" << QString::fromStdString(filename);
            return;
        }
        if (is_visible_app(keyFile) == false) {
            return; // from the enumerator lambda
        }

        if (include_desktop_results || keyFile.has_key(DESKTOP_FILE_GROUP, DESKTOP_FILE_UBUNTU_TOUCH)
            || keyFile.has_key(DESKTOP_FILE_GROUP, DESKTOP_FILE_KEY_APP_ID)
            || Interface::is_non_click_app(QString::fromStdString(filename))) {
            auto app = load_app_from_desktop(keyFile, filename);
            QString title = QString::fromStdString(app.title);
            if (search_query.empty() ||
                (!title.isEmpty() && title.contains(search_query.c_str(),
                                                    Qt::CaseInsensitive))) {
                result.push_back(app);
                qDebug() << QString::fromStdString(app.title) << QString::fromStdString(app.icon_url) << QString::fromStdString(filename);
            }
        }
    };

    keyFileLocator->enumerateKeyFilesForInstalledApplications(enumerator);
    // Sort applications so that newest come first.
    std::sort(result.begin(), result.end(), [](const Application& a,
                                               const Application& b) {
                  return a.installed_time > b.installed_time;
              });
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

ManifestList manifest_list_from_json(const std::string& json)
{
    using namespace boost::property_tree;

    std::istringstream is(json);

    ptree pt;
    read_json(is, pt);

    ManifestList manifests;

    BOOST_FOREACH(ptree::value_type &v, pt)
    {
        assert(v.first.empty()); // array elements have no names
        auto node = v.second;
        Manifest manifest;

        manifest.name = node.get<std::string>("name");
        manifest.version = node.get<std::string>("version");
        manifest.removable = node.get<bool>("_removable");

        BOOST_FOREACH(ptree::value_type &sv, node.get_child("hooks"))
        {
            // FIXME: "primary app" for a package is not defined, we just
            // use first one here:
            manifest.first_app_name = sv.first;
            break;
        }
        qDebug() << "adding manifest: " << manifest.name.c_str() << manifest.version.c_str() << manifest.first_app_name.c_str();

        manifests.push_back(manifest);
    }

    return manifests;
}

Manifest manifest_from_json(const std::string& json)
{
    using namespace boost::property_tree;

    std::istringstream is(json);

    ptree pt;
    read_json(is, pt);

    Manifest manifest;

    manifest.name = pt.get<std::string>("name");
    manifest.version = pt.get<std::string>("version");
    manifest.removable = pt.get<bool>("_removable");

    BOOST_FOREACH(ptree::value_type &sv, pt.get_child("hooks"))
    {
        // FIXME: "primary app" for a package is not defined, we just
        // use first one here:
        manifest.first_app_name = sv.first;
        break;
    }
    qDebug() << "adding manifest: " << manifest.name.c_str() << manifest.version.c_str() << manifest.first_app_name.c_str();

    return manifest;
}

void Interface::get_manifests(std::function<void(ManifestList, ManifestError)> callback)
{
    std::string command = "click list --manifest";
    qDebug() << "Running command:" << command.c_str();
    run_process(command, [callback](int code, const std::string& stdout_data, const std::string&) {
        if (code == 0) {
            try {
                ManifestList manifests = manifest_list_from_json(stdout_data);
                callback(manifests, ManifestError::NoError);
            } catch (...) {
                callback(ManifestList(), ManifestError::ParseError);
            }
        } else {
            callback(ManifestList(), ManifestError::CallError);
        }
    });
}

void Interface::get_manifest_for_app(const std::string &app_id,
                                     std::function<void(Manifest, ManifestError)> callback)
{
    std::string command = "click info " + app_id;
    qDebug() << "Running command:" << command.c_str();
    run_process(command, [callback](int code, const std::string& stdout_data, const std::string&) {
        if (code == 0) {
            try {
                Manifest manifest = manifest_from_json(stdout_data);
                callback(manifest, ManifestError::NoError);
            } catch (...) {
                callback(Manifest(), ManifestError::ParseError);
            }
        } else {
            callback(Manifest(), ManifestError::CallError);
        }
    });
}

void Interface::get_dotdesktop_filename(const std::string &app_id,
                                        std::function<void(std::string, ManifestError)> callback)
{
    get_manifest_for_app(app_id, [app_id, callback] (Manifest manifest, ManifestError error) {
        qDebug() << "in get_dotdesktop_filename callback";

        if (error != ManifestError::NoError){
            callback(std::string("Internal Error"), error);
            return;
        }
        qDebug() << "in get_dotdesktop_filename callback";

        if (!manifest.name.empty()) {
            std::string ddstr = manifest.name + "_" + manifest.first_app_name + "_" + manifest.version + ".desktop";
            callback(ddstr, ManifestError::NoError);
        } else {
            qCritical() << "Warning: no manifest found for " << app_id.c_str();
            callback(std::string("Not found"), ManifestError::CallError);
        }
    });
}

void Interface::run_process(const std::string& command,
                            std::function<void(int code,
                                               const std::string& stdout_data,
                                               const std::string& stderr_data)> callback)
{
    QSharedPointer<QProcess> process(new QProcess());
    typedef void(QProcess::*QProcessFinished)(int, QProcess::ExitStatus);
    typedef void(QProcess::*QProcessError)(QProcess::ProcessError);
    QObject::connect(process.data(),
                     static_cast<QProcessFinished>(&QProcess::finished),
                     [callback, process](int code, QProcess::ExitStatus /*status*/) {
                         qDebug() << "command finished with exit code:" << code;
                         auto data = process.data()->readAllStandardOutput().data();
                         auto errors = process.data()->readAllStandardError().data();
                         callback(code, data, errors);
                     } );

    QObject::connect(process.data(),
                     static_cast<QProcessError>(&QProcess::error),
                     [callback, process](QProcess::ProcessError error) {
                         qCritical() << "error running command:" << error;
                         auto data = process.data()->readAllStandardOutput().data();
                         auto errors = process.data()->readAllStandardError().data();
                         callback(process.data()->exitCode(), data, errors);
                     } );

    process->start(command.c_str());
}

} // namespace click
