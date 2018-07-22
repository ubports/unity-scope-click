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

#ifndef CLICK_APPLICATION_H
#define CLICK_APPLICATION_H

#include "index.h"

namespace click
{

struct Application : public Package {
    Application(std::string name,
                std::string title,
                double price,
                std::string icon_url,
                std::string url,
                std::string description,
                std::string main_screenshot,
                std::string default_department,
                std::string real_department = ""
                ) : Package {name, title, price, icon_url, url},
                    description(description),
                    main_screenshot(main_screenshot),
                    default_department(default_department),
                    real_department(real_department)
    {

    }
    Application() = default;
    ~Application() {}

    std::string description;
    std::vector<std::string> keywords;
    std::string main_screenshot;
    std::string default_department;
    std::string real_department;
    bool is_legacy = false;
    time_t installed_time;
};

std::ostream& operator<<(std::ostream& out, const Application& app);

bool operator==(const Application& lhs, const Application& rhs);

} // namespace click

#endif // CLICK_APPLICATION_H
