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

#ifndef UNITY_CLICK_INTERFACE_H
#define UNITY_CLICK_INTERFACE_H

#include <list>
#include <string>
#include "clickwebservice.h"

namespace Unity {
namespace Click {

class Application
{
public:
    void get_package(std::function<Package> callback);
//    void get_manifest(std::function<JsonNode> callback); // pending json library
    void get_dotdesktop(std::function<std::string> callback);
    void can_uninstall(std::function<bool> callback);
    void uninstall(std::function<bool> callback);
};

class Interface
{
public:
    Interface();
    std::string get_arch();
    std::string get_frameworks();
//    void get_manifests(std::function<list<JsonNode>> callback); // pending json library
    void get_installed(std::string search_query, std::function<std::list<Application>> callback);
};

} // namespace Click
} // namespace Unity

#endif // UNITY_CLICK_INTERFACE_H
