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

#include <string>
#include <vector>

#include <QDir>
#include <QProcess>
#include <QStringList>
#include <QVariant>
#include <QDebug>

#include <qgsettings.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "configuration.h"

namespace click {

/* NOTE: The list of languages we need to use the full language code for.
 * Please keep this list in a-z order.
 */
const std::vector<const char*> Configuration::FULL_LANG_CODES = {
    "pt_BR",
    "zh_CN",
    "zh_TW",
};

std::vector<std::string> Configuration::list_folder(const std::string& folder, const std::string& pattern)
{
    std::vector<std::string> result;

    QDir dir(QString::fromStdString(folder), QString::fromStdString(pattern),
             QDir::Unsorted, QDir::Readable | QDir::Files);
    QStringList entries = dir.entryList();
    for (int i = 0; i < entries.size(); ++i) {
        QString filename = entries.at(i);
        result.push_back(filename.toStdString());
    }

    return result;
}

std::vector<std::string> Configuration::get_available_frameworks()
{
    std::vector<std::string> result;
    for (auto f: list_folder(FRAMEWORKS_FOLDER, FRAMEWORKS_PATTERN)) {
        result.push_back(f.substr(0, f.size()-FRAMEWORKS_EXTENSION_LENGTH));
    }
    return result;
}

std::string Configuration::architectureFromDpkg()
{
    QString program("dpkg");
    QStringList arguments;
    arguments << "--print-architecture";
    QProcess archDetector;
    archDetector.start(program, arguments);
    if(!archDetector.waitForFinished()) {
        throw std::runtime_error("Architecture detection failed.");
    }
    auto output = archDetector.readAllStandardOutput();
    auto ostr = QString::fromUtf8(output);
    ostr.remove('\n');

    return ostr.toStdString();
}

std::string Configuration::get_architecture()
{
    const char* env_arch = getenv(ARCH_ENVVAR);
    static const std::string deb_arch {architectureFromDpkg()};
    if (env_arch == NULL) {
        return deb_arch;
    }
    static const std::string arch {env_arch};
    return arch;
}

std::string Configuration::get_language_base()
{
    std::string language = get_language();
    std::vector<std::string> lang_parts;
    boost::split(lang_parts, language, boost::is_any_of("_"));
    return lang_parts[0];
}

std::string Configuration::get_language()
{
    const char* language = getenv(LANGUAGE_ENVVAR);
    if (language == NULL) {
        language = "C";
    }
    std::vector<std::string> lang_parts;
    boost::split(lang_parts, language, boost::is_any_of("."));
    return lang_parts[0];
}

std::string Configuration::get_accept_languages()
{
    std::string language = get_language();
    std::vector<std::string> lang_parts;
    boost::split(lang_parts, language, boost::is_any_of("_"));
    std::string result;
    if (lang_parts.size() > 1) {
        boost::replace_first(language, "_", "-");
        result = language + ", " + get_language_base();
    } else {
        result = language;
    }
    return result;
}

bool Configuration::is_full_lang_code(const std::string& language)
{
    return std::find(FULL_LANG_CODES.begin(), FULL_LANG_CODES.end(), language)
        != FULL_LANG_CODES.end();
}

const std::vector<std::string> Configuration::get_dconf_strings(const std::string& schema, const std::string &path, const std::string& key) const
{
    if (!QGSettings::isSchemaInstalled(schema.c_str()))
    {
        qWarning() << "Schema" << QString::fromStdString(schema) << "is missing";
        return std::vector<std::string>();
    }
    QGSettings qgs(schema.c_str(), path.c_str());
    std::vector<std::string> v;
    if (qgs.keys().contains(QString::fromStdString(key)))
    {
        auto locations = qgs.get(QString::fromStdString(key)).toStringList();
        for(const auto& l : locations) {
            v.push_back(l.toStdString());
        }
    }
    else
    {
        qWarning() << "No" << QString::fromStdString(key) << " key in schema" << QString::fromStdString(schema);
    }
    return v;
}

const std::vector<std::string> Configuration::get_core_apps() const
{
    auto apps = get_dconf_strings(Configuration::COREAPPS_SCHEMA, Configuration::COREAPPS_PATH, Configuration::COREAPPS_KEY);
    if (apps.empty()) {
        apps = get_default_core_apps();
    }
    return apps;
}

} // namespace click
