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

#ifndef CLICK_PACKAGE_H
#define CLICK_PACKAGE_H

#include <string>
#include <list>

#include <json/json.h>


namespace json = Json;

namespace click {

struct Package
{
    struct JsonKeys
    {
        JsonKeys() = delete;

        constexpr static const char* embedded {"_embedded"};
        constexpr static const char* links{"_links"};
        constexpr static const char* ci_package {"clickindex:package"};
        constexpr static const char* name{"name"};
        constexpr static const char* title{"title"};
        constexpr static const char* price{"price"};
        constexpr static const char* icon_url{"icon_url"};
        constexpr static const char* resource_url{"href"};
    };

    Package() = default;
    Package(std::string name, std::string title, double price, std::string icon_url, std::string url) :
        name(name),
        title(title),
        price(price),
        icon_url(icon_url),
        url(url)
    {
    }
    Package(std::string name, std::string title, double price, std::string icon_url, std::string url, std::string version) :
        name(name),
        title(title),
        price(price),
        icon_url(icon_url),
        url(url),
        version(version)
    {
    }
    virtual ~Package() = default;

    std::string name; // formerly app_id
    std::string title;
    double price;
    std::string icon_url;
    std::string url;
    std::string version;
    void matches (std::string query, std::function<bool> callback);
};

typedef std::list<Package> PackageList;

PackageList package_list_from_json(const std::string& json);
PackageList package_list_from_json_node(const Json::Value& root);

struct PackageDetails
{
    struct JsonKeys
    {
        JsonKeys() = delete;

        constexpr static const char* name{"name"};
        constexpr static const char* title{"title"};
        constexpr static const char* icon_url{"icon_url"};
        constexpr static const char* description{"description"};
        constexpr static const char* download_url{"download_url"};
        constexpr static const char* rating{"rating"};
        constexpr static const char* keywords{"keywords"};
        constexpr static const char* terms_of_service{"terms_of_service"};
        constexpr static const char* license{"license"};
        constexpr static const char* publisher{"publisher"};
        constexpr static const char* main_screenshot_url{"screenshot_url"};
        constexpr static const char* more_screenshot_urls{"screenshot_urls"};
        constexpr static const char* binary_filesize{"binary_filesize"};
        constexpr static const char* version{"version"};
        constexpr static const char* framework{"framework"};
    };

    static PackageDetails from_json(const std::string &json);

    Package package;

    std::string description;
    std::string download_url;
    double rating;
    std::string keywords;
    std::string terms_of_service;
    std::string license;
    std::string publisher;
    std::string main_screenshot_url;
    std::list<std::string> more_screenshots_urls;
    json::Value::UInt64 binary_filesize;
    std::string version;
    std::string framework;
};

std::ostream& operator<<(std::ostream& out, const PackageDetails& details);

bool operator==(const Package& lhs, const Package& rhs);
bool operator==(const PackageDetails& lhs, const PackageDetails& rhs);

} // namespace click

#endif // CLICK_PACKAGE_H
