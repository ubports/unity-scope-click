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

#include "key_file_locator.h"

#include <unity/Exception.h>
#include <unity/UnityExceptions.h>
#include <unity/util/IniParser.h>

#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QString>

namespace
{
static const QString NON_CLICK_PATH("/usr/share/applications");

void find_apps_in_dir(const QString& dir_path,
                                 const click::KeyFileLocator::Enumerator& enumerator)
{
    QDir dir(dir_path, "*.desktop",
             QDir::Unsorted, QDir::Readable | QDir::Files);
    QStringList entries = dir.entryList();
    for (int i = 0; i < entries.size(); ++i) {
        QString filename = entries.at(i);
        QString full_path = dir.absoluteFilePath(filename);
        try {
            enumerator(unity::util::IniParser(full_path.toUtf8().data()),
                       filename.toUtf8().data());
        } catch (const unity::FileException& file_exp) {
            qWarning() << "Error reading file:" << file_exp.to_string().c_str();
        } catch (const unity::LogicException& logic_exp) {
            qCritical() << "Error reading file:" << logic_exp.to_string().c_str();
        }
    }
}
}

const std::string& click::KeyFileLocator::systemApplicationsDirectory()
{
    static const std::string s{"/usr/share/applications"};
    return s;
}

const std::string& click::KeyFileLocator::userApplicationsDirectory()
{
    static const std::string s
    {
        qPrintable(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/applications")
    };
    return s;
}

click::KeyFileLocator::KeyFileLocator(
        const std::string& systemApplicationsDir,
        const std::string& userApplicationsDir)
    : systemApplicationsDir(systemApplicationsDir),
      userApplicationsDir(userApplicationsDir)
{
}

void click::KeyFileLocator::enumerateKeyFilesForInstalledApplications(
        const click::KeyFileLocator::Enumerator& enumerator)
{
    find_apps_in_dir(QString::fromStdString(systemApplicationsDir), enumerator);
    find_apps_in_dir(QString::fromStdString(userApplicationsDir), enumerator);
}
