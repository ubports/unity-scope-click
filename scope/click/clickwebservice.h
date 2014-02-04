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

#ifndef CLICKWEBSERVICE_H
#define CLICKWEBSERVICE_H

#include <string>
#include <list>
#include <functional>

namespace click {

struct Package
{
    std::string name; // formerly app_id
    std::string title;
    std::string price;
    std::string icon_url;
    std::string url;
    void matches (std::string query, std::function<bool> callback);
};

struct PackageDetails
{
    std::string name; // formerly app_id
    std::string icon_url;
    std::string title;
    std::string description;
    std::string download_url;
    std::string rating;
    std::string keywords;
    std::string terms_of_service;
    std::string license;
    std::string publisher;
    std::string main_screenshot_url;
    std::string more_screenshots_urls;
    std::string binary_filesize;
    std::string version;
    std::string framework;
    static void from_json(std::string json);
};

class Webservice
{
public:
    Webservice();
    void search (std::string query, std::function<std::list<Package>> callback);
};

} // namespace click

#endif // CLICKWEBSERVICE_H

