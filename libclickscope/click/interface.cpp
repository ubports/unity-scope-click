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

#include <cstdio>
#include <list>
#include <sys/stat.h>
#include <map>
#include <sstream>

#include <boost/locale/collator.hpp>
#include <boost/locale/generator.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/exceptions.hpp>
#include <boost/foreach.hpp>

#include <unity/UnityExceptions.h>
#include <unity/util/IniParser.h>

#include "interface.h"
#include <click/key_file_locator.h>
#include <click/departments-db.h>

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
static const std::string DESKTOP_FILE_KEY_KEYWORDS("Keywords");
static const std::string DESKTOP_FILE_KEY_APP_ID("X-Ubuntu-Application-ID");
static const std::string DESKTOP_FILE_KEY_DOMAIN("X-Ubuntu-Gettext-Domain");
static const std::string DESKTOP_FILE_UBUNTU_TOUCH("X-Ubuntu-Touch");
static const std::string DESKTOP_FILE_UBUNTU_DEFAULT_DEPARTMENT("X-Ubuntu-Default-Department-ID");
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

    app.url = "application:///" + filename;
    if (keyFile.has_key(DESKTOP_FILE_GROUP, DESKTOP_FILE_KEY_ICON)) {
        app.icon_url = add_theme_scheme(keyFile.get_string(DESKTOP_FILE_GROUP,
                                                           DESKTOP_FILE_KEY_ICON));
    }

    if (keyFile.has_key(DESKTOP_FILE_GROUP, DESKTOP_FILE_KEY_KEYWORDS)) {
        app.keywords = keyFile.get_string_array(DESKTOP_FILE_GROUP,
                                                DESKTOP_FILE_KEY_KEYWORDS);
    }

    if (keyFile.has_key(DESKTOP_FILE_GROUP, DESKTOP_FILE_UBUNTU_DEFAULT_DEPARTMENT)) {
        app.default_department = keyFile.get_string(DESKTOP_FILE_GROUP, DESKTOP_FILE_UBUNTU_DEFAULT_DEPARTMENT);
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

std::vector<click::Application> Interface::sort_apps(const std::vector<click::Application>& apps)
{
    std::vector<click::Application> result = apps;
    boost::locale::generator gen;
    const char* lang = getenv(click::Configuration::LANGUAGE_ENVVAR);
    if (lang == NULL) {
        lang = "C.UTF-8";
    }
    std::locale loc = gen(lang);
    std::locale::global(loc);
    typedef boost::locale::collator<char> coll_type;

    // Sort applications alphabetically.
    std::sort(result.begin(), result.end(), [&loc](const Application& a,
                                                   const Application& b) {
                  bool lesser = false;
                  int order = std::use_facet<coll_type>(loc)
                      .compare(boost::locale::collator_base::quaternary,
                               a.title, b.title);
                  if (order == 0) {
                      lesser = a.name < b.name;
                  } else {
                      // Because compare returns int, not bool, we have to check
                      // that 0 is greater than the result, which tells us the
                      // first element should be sorted priori
                      lesser = order < 0;
                  }
                  return lesser;
              });

    return result;
}

/* find_installed_apps()
 *
 * Find all of the installed apps matching @search_query in a timeout.
 */
std::vector<click::Application> Interface::find_installed_apps(const std::string& search_query,
        const std::string& current_department,
        const std::shared_ptr<click::DepartmentsDb>& depts_db)
{
    //
    // only apply department filtering if not in root of all departments.
    bool apply_department_filter = (search_query.empty() && !current_department.empty());

    // get the set of packages that belong to current deparment;
    std::unordered_set<std::string> packages_in_department;
    if (depts_db && apply_department_filter)
    {
        try
        {
            packages_in_department = depts_db->get_packages_for_department(current_department);
        }
        catch (const std::exception& e)
        {
            qWarning() << "Failed to get packages of department" << QString::fromStdString(current_department);
            apply_department_filter = false; // disable so that we are not loosing any apps if something goes wrong
        }
    }

    std::vector<Application> result;

    bool include_desktop_results = show_desktop_apps();

    auto enumerator = [&result, this, search_query, current_department, packages_in_department, apply_department_filter, include_desktop_results, depts_db]
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

            // check if apps is present in current department
            if (apply_department_filter)
            {
                // app from click package has non-empty name; for non-click apps use desktop filename
                const std::string key = app.name.empty() ? filename : app.name;
                if (packages_in_department.find(key) == packages_in_department.end())
                {
                    if (app.default_department.empty())
                    {
                        // default department not present in the keyfile, skip this app
                        return;
                    }
                    else
                    {
                        // default department not empty: check if this app is in a different
                        // department in the db (i.e. got moved from the default department);
                        if (depts_db->has_package(key))
                        {
                            // app is now in a different department
                            return;
                        }

                        if (app.default_department != current_department)
                        {
                            return;
                        }

                        // else - this package is in current department
                    }
                }
            }

            if (search_query.empty()) {
                result.push_back(app);
            } else {
                std::string lquery = search_query;
                std::transform(lquery.begin(), lquery.end(),
                               lquery.begin(), ::tolower);
                // Check keywords for the search query as well.
                for (auto keyword: app.keywords) {
                    std::transform(keyword.begin(), keyword.end(),
                                   keyword.begin(), ::tolower);
                    if (!keyword.empty()
                        && keyword.find(lquery) != std::string::npos) {
                        result.push_back(app);
                        return;
                    }
                }

                std::string search_title = app.title;
                std::transform(search_title.begin(), search_title.end(),
                               search_title.begin(), ::tolower);
                // check the app title for the search query.
                if (!search_title.empty()
                    && search_title.find(lquery) != std::string::npos) {
                    result.push_back(app);
                }
            }
        }
    };

    keyFileLocator->enumerateKeyFilesForInstalledApplications(enumerator);
    return sort_apps(result);
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
        // FIXME: "primary app or scope" for a package is not defined,
        // we just use the first one in the manifest:
        auto app_name = sv.second.get("desktop", "");
        if (manifest.first_app_name.empty() && !app_name.empty()) {
            manifest.first_app_name = sv.first;
        }
        auto scope_id = sv.second.get("scope", "");
        if (manifest.first_scope_id.empty() && !scope_id.empty()) {
            manifest.first_scope_id = manifest.name + "_" + sv.first;
        }
    }
    qDebug() << "adding manifest: " << manifest.name.c_str() << manifest.version.c_str() << manifest.first_app_name.c_str();

    return manifest;
}

void Interface::get_manifests(std::function<void(ManifestList, InterfaceError)> callback)
{
    std::string command = "click list --manifest";
    qDebug() << "Running command:" << command.c_str();
    run_process(command, [callback](int code, const std::string& stdout_data, const std::string& stderr_data) {
        if (code == 0) {
            try {
                ManifestList manifests = manifest_list_from_json(stdout_data);
                callback(manifests, InterfaceError::NoError);
            } catch (...) {
                qWarning() << "Can't parse 'click list --manifest' output: " << QString::fromStdString(stdout_data);
                callback(ManifestList(), InterfaceError::ParseError);
            }
        } else {
            qWarning() << "Error" << code << "running 'click list --manifest': " << QString::fromStdString(stderr_data);
            callback(ManifestList(), InterfaceError::CallError);
        }
    });
}

PackageSet package_names_from_stdout(const std::string& stdout_data)
{
    const char TAB='\t', NEWLINE='\n';
    std::istringstream iss(stdout_data);
    PackageSet installed_packages;

    while (iss.peek() != EOF) {
        std::string line;
        std::getline(iss, line, NEWLINE);
        // Must initialize linestream after line is filled.
        std::istringstream linestream(line);

        Package p;
        std::getline(linestream, p.name, TAB);
        std::getline(linestream, p.version, NEWLINE);
        if (iss.eof() || p.name.empty() || p.version.empty()) {
            qWarning() << "Error encountered parsing 'click list' output:" << QString::fromStdString(line);
            continue;
        }
        installed_packages.insert(p);
    }

    return installed_packages;
}

void Interface::get_installed_packages(std::function<void(PackageSet, InterfaceError)> callback)
{
    std::string command = "click list";
    qDebug() << "Running command:" << command.c_str();
    run_process(command, [callback](int code, const std::string& stdout_data, const std::string& stderr_data) {
        if (code == 0) {
            try {
                PackageSet package_names = package_names_from_stdout(stdout_data);
                callback(package_names, InterfaceError::NoError);
            } catch (...) {
                qWarning() << "Can't parse 'click list' output: " << QString::fromStdString(stdout_data);
                callback(PackageSet(), InterfaceError::ParseError);
            }
        } else {
            qWarning() << "Error" << code << "running 'click list': " << QString::fromStdString(stderr_data);
            callback(PackageSet(), InterfaceError::CallError);
        }
    });
}

void Interface::get_manifest_for_app(const std::string &app_id,
                                     std::function<void(Manifest, InterfaceError)> callback)
{
    std::string command = "click info " + app_id;
    qDebug() << "Running command:" << command.c_str();
    run_process(command, [callback, app_id](int code, const std::string& stdout_data, const std::string& stderr_data) {
        if (code == 0) {
            try {
                Manifest manifest = manifest_from_json(stdout_data);
                callback(manifest, InterfaceError::NoError);
            } catch (...) {
                qWarning() << "Can't parse 'click info" << QString::fromStdString(app_id)
                           << "' output: " << QString::fromStdString(stdout_data);
                callback(Manifest(), InterfaceError::ParseError);
            }
        } else {
            qWarning() << "Error" << code << "running 'click info" << QString::fromStdString(app_id)
                       << "': " << QString::fromStdString(stderr_data);
            callback(Manifest(), InterfaceError::CallError);
        }
    });
}

void Interface::get_dotdesktop_filename(const std::string &app_id,
                                        std::function<void(std::string, InterfaceError)> callback)
{
    get_manifest_for_app(app_id, [app_id, callback] (Manifest manifest, InterfaceError error) {
        qDebug() << "in get_dotdesktop_filename callback";

        if (error != InterfaceError::NoError){
            callback(std::string("Internal Error"), error);
            return;
        }
        qDebug() << "in get_dotdesktop_filename callback";

        if (!manifest.name.empty()) {
            std::string ddstr = manifest.name + "_" + manifest.first_app_name + "_" + manifest.version + ".desktop";
            callback(ddstr, InterfaceError::NoError);
        } else {
            qCritical() << "Warning: no manifest found for " << app_id.c_str();
            callback(std::string("Not found"), InterfaceError::CallError);
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
