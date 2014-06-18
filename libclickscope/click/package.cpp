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

#include <sstream>

#include <click/application.h>
#include <click/package.h>

namespace click
{

QDebug operator<< (QDebug d, const std::string &s) {
    d << QString::fromStdString(s);
    return d;
}

bool operator==(const Package& lhs, const Package& rhs) {
    // We can't include the version in the comparison here, because this
    // comparison is used by the sorted_set, that we use to compare a package
    // installed locally on the device with a (possibly updated) package available in the store.
    return lhs.name == rhs.name;

}

bool operator==(const PackageDetails& lhs, const PackageDetails& rhs) {
    return lhs.package == rhs.package &&
            lhs.description == rhs.description &&
            lhs.download_url == rhs.download_url &&
            lhs.rating == rhs.rating &&
            // TODO: keywords should be a list of strings
            lhs.keywords == rhs.keywords &&
            lhs.terms_of_service == rhs.terms_of_service &&
            lhs.license == rhs.license &&
            lhs.publisher == rhs.publisher &&
            lhs.main_screenshot_url == rhs.main_screenshot_url &&
            // TODO: more_screenshots_urls should be a list of strings
            lhs.more_screenshots_urls == rhs.more_screenshots_urls &&
            // TODO: binary_filesize should be a int/long
            lhs.binary_filesize == rhs.binary_filesize &&
            lhs.version == rhs.version &&
            lhs.framework == rhs.framework;
}

Package package_from_json_node(const Json::Value& item)
{
    Package p;
    p.name = item[Package::JsonKeys::name].asString();
    p.title = item[Package::JsonKeys::title].asString();
    p.price = item[Package::JsonKeys::price].asDouble();
    p.icon_url = item[Package::JsonKeys::icon_url].asString();
    p.url = item[Package::JsonKeys::links][Package::JsonKeys::self][Package::JsonKeys::href].asString();
    if (p.url.empty()) {
        p.url = item[Package::JsonKeys::resource_url].asString();
    }
    return p;
}

Packages package_list_from_json_node(const Json::Value& root)
{
    Packages pl;
    for (uint i = 0; i < root.size(); i++)
    {
        Package p;
        const json::Value item = root[i];
        p = package_from_json_node(item);
        pl.push_back(p);
    }
    return pl;
}

Packages package_list_from_json(const std::string& json)
{
    std::istringstream is(json);

    json::Reader reader;
    json::Value root;

    if (!reader.parse(json, root)) {
        throw std::runtime_error(reader.getFormattedErrorMessages());
    }

    return package_list_from_json_node(root);
}

PackageDetails PackageDetails::from_json(const std::string &json)
{
    PackageDetails details;
    try
    {
        json::Reader reader;
        json::Value root;

        if (!reader.parse(json, root))
            throw std::runtime_error(reader.getFormattedErrorMessages());

        // Mandatory details go here. That is, asString() will throw as we
        // do not provide a default value if a value with the given key does not exist.
        details.package.name = root[Package::JsonKeys::name].asString();
        details.package.title = root[Package::JsonKeys::title].asString();
        details.package.icon_url = root[Package::JsonKeys::icon_url].asString();
        details.package.price = root[Package::JsonKeys::price].asDouble();
        details.description = root[JsonKeys::description].asString();
        details.download_url = root[JsonKeys::download_url].asString();
        details.license = root[JsonKeys::license].asString();

        // Optional details go here.
        if (root[JsonKeys::version].isString())
            details.version = root[JsonKeys::version].asString();

        if (root[JsonKeys::rating].isNumeric())
            details.rating = root[JsonKeys::rating].asDouble();

        if (root[JsonKeys::keywords].isString())
            details.keywords = root[JsonKeys::keywords].asString();

        if (root[JsonKeys::terms_of_service].isString())
            details.terms_of_service = root[JsonKeys::terms_of_service].asString();

        if (root[JsonKeys::publisher].isString())
            details.publisher = root[JsonKeys::publisher].asString();

        if (root[JsonKeys::main_screenshot_url].isString())
            details.main_screenshot_url = root[JsonKeys::main_screenshot_url].asString();

        auto screenshots = root[JsonKeys::more_screenshot_urls];

        for (uint i = 0; i < screenshots.size(); i++)
        {
            auto scr = screenshots[i].asString();
            // more_screenshot_urls may contain main_screenshot_url, if so, skip it
            if (scr != details.main_screenshot_url)
            {
                details.more_screenshots_urls.push_back(scr);
            }
        }

        if (root[JsonKeys::binary_filesize].isIntegral())
            details.binary_filesize = root[JsonKeys::binary_filesize].asUInt64();

        if (root[JsonKeys::framework].isString())
            details.framework = root[JsonKeys::framework].asString();

    } catch(const std::exception& e)
    {
        std::cerr << "PackageDetails::loadJson: Exception thrown while decoding JSON: " << e.what() << std::endl;
    } catch(...)
    {
        std::cerr << "PackageDetails::loadJson: Exception thrown while decoding JSON." << std::endl;
    }

    return details;
}

std::string print_string_if_not_empty(const std::string& s)
{

    return s.empty() ? "n/a" : s;
}

std::string print_list_if_not_empty(const std::list<std::string>& li)
{
    std::stringstream s;
    s << "[";
    if (!li.empty())
    {
        auto it = li.begin();
        s << print_string_if_not_empty(*it);
        ++it;
        while (it != li.end())
        {
            s << ", " << print_string_if_not_empty(*it);
            ++it;
        }
    }
    s << "]";
    return s.str();
}

std::ostream& operator<<(std::ostream& out, const click::PackageDetails& details)
{
    out << "("
        << print_string_if_not_empty(details.package.name) << ", "
        << print_string_if_not_empty(details.package.title) << ", "
        << print_string_if_not_empty(details.package.icon_url) << ", "
        << print_string_if_not_empty(details.description) << ", "
        << print_string_if_not_empty(details.download_url) << ", "
        << details.rating << ", "
        << print_string_if_not_empty(details.keywords) << ", "
        << print_string_if_not_empty(details.terms_of_service) << ", "
        << print_string_if_not_empty(details.license) << ", "
        << print_string_if_not_empty(details.publisher) << ", "
        << print_string_if_not_empty(details.main_screenshot_url) << ", "
        << print_list_if_not_empty(details.more_screenshots_urls) << ", "
        << details.binary_filesize << ", "
        << print_string_if_not_empty(details.version) << ", "
        << print_string_if_not_empty(details.framework)
        << ")";

    return out;
}

bool operator==(const Application& lhs, const Application& rhs) {
    return lhs.name == rhs.name &&
            lhs.title == rhs.title &&
            lhs.description == rhs.description &&
            lhs.main_screenshot == rhs.main_screenshot &&
            lhs.icon_url == rhs.icon_url;
}

std::ostream& operator<<(std::ostream& out, const click::Application& app)
{
    out << "("
        << print_string_if_not_empty(app.name) << ", "
        << print_string_if_not_empty(app.title) << ", "
        << app.price << ", "
        << print_string_if_not_empty(app.icon_url) << ", "
        << print_string_if_not_empty(app.url) << ", "
        << print_string_if_not_empty(app.version) << ", "
        << print_string_if_not_empty(app.description) << ", "
        << print_string_if_not_empty(app.main_screenshot)
        << ")";

    return out;
}

} // namespace click
