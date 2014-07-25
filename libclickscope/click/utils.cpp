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

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/time_facet.hpp>
#include <boost/locale.hpp>
#include <boost/units/systems/si/io.hpp>

#include <click/utils.h>

struct byte_base_unit : boost::units::base_unit<byte_base_unit, boost::units::dimensionless_type, 3>
{
  static const char* name() { return("byte"); }
  static const char* symbol() { return("B"); }
};

std::string click::Formatter::human_readable_filesize(long num_bytes)
{
    std::ostringstream s;
    std::cout.imbue(std::locale());

    if (num_bytes > 1023) {
        s << boost::units::symbol_format << boost::units::binary_prefix;
        s << boost::locale::format("{1,num=fixed,precision=1}") % (num_bytes * byte_base_unit::unit_type());
    } else {
        std::string tpl(dngettext(GETTEXT_PACKAGE, "{1} byte", "{1} bytes", num_bytes));
        s << boost::locale::format(tpl) % num_bytes;
    }
    return s.str();
}

using namespace boost::posix_time;

time_input_facet* build_input_facet(std::stringstream& ss)
{
    time_input_facet* input_facet = new time_input_facet(1);
    input_facet->set_iso_extended_format();
    ss.imbue(std::locale(ss.getloc(), input_facet));
    return input_facet;
}

void click::Date::setup_system_locale()
{
    boost::locale::generator gen;
    std::locale loc=gen("");
    std::locale::global(loc);
}

void click::Date::parse_iso8601(std::string iso8601)
{
    static std::stringstream ss;
    static ptime epoch(boost::gregorian::date(1970,1,1));
    static time_input_facet* input_facet = NULL;

    if (input_facet == NULL) {
        build_input_facet(ss);
    }

    ptime time;
    ss.str(iso8601);
    ss >> time;
    ss.clear();

    timestamp = (time - epoch).total_seconds();
}

std::string click::Date::formatted() const
{
    std::stringstream s;
    s << boost::locale::as::date << timestamp;
    return s.str();
}
