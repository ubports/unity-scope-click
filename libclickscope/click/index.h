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

#ifndef CLICKINDEX_H
#define CLICKINDEX_H


#include <string>
#include <functional>

#include <click/configuration.h>

#include "package.h"
#include <click/departments.h>
#include <click/highlights.h>

#include <QSharedPointer>


namespace click {

class Configuration;

class PackageManager
{
public:
    void uninstall (const Package& package, std::function<void(int, std::string)>);
    virtual void execute_uninstall_command (const std::string& command, std::function<void(int, std::string)>);
    static void invalidate_results(const std::string& scope_id);
};

class Index
{
protected:
    QSharedPointer<Configuration> configuration;
    std::string m_suggested_currency;
    virtual std::string build_index_query(const std::string& query, const std::string& department);
    virtual std::map<std::string, std::string> build_headers();

public:
    enum class Error {NoError, CredentialsError, NetworkError};
    Index() {}
    Index(const QSharedPointer<Configuration> configuration=QSharedPointer<Configuration>(new Configuration()));
    virtual std::pair<Packages, Packages> package_lists_from_json(const std::string& json);
    virtual ~Index();

};

} // namespace click

#endif // CLICKINDEX_H

