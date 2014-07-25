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

#include <gtest/gtest.h>
#include <boost/locale.hpp>
#include <boost/locale/time_zone.hpp>

#include <click/utils.h>

using namespace ::testing;

class UtilsTest : public Test
{
public:
    void SetUp()
    {
        // The tests assume they run always under the same environment
        setDefaultTimezone();
        setDefaultLocale();
    }

    void setDefaultTimezone()
    {
        boost::locale::time_zone::global("UTC");
    }

    void setDefaultLocale()
    {
        boost::locale::generator gen;
        std::locale::global(gen("C"));
    }
};

TEST_F(UtilsTest, testHumanReadableZeroBytes)
{
    ASSERT_EQ(click::Formatter::human_readable_filesize(0), "0 bytes");
}

TEST_F(UtilsTest, testHumanReadableOneByte)
{
    ASSERT_EQ(click::Formatter::human_readable_filesize(1), "1 byte");
}

TEST_F(UtilsTest, testHumanReadableMoreBytes)
{
    ASSERT_EQ(click::Formatter::human_readable_filesize(102), "102 bytes");
}

TEST_F(UtilsTest, testHumanReadableKilobytes)
{
    ASSERT_EQ(click::Formatter::human_readable_filesize(102*1024), "102.0 KiB");
}

TEST_F(UtilsTest, testHumanReadableFractionalKilobytes)
{
    ASSERT_EQ(click::Formatter::human_readable_filesize(102*1024+512+1), "102.5 KiB");
}

TEST_F(UtilsTest, testHumanReadableMegabytes)
{
    ASSERT_EQ(click::Formatter::human_readable_filesize(42*1024*1024), "42.0 MiB");
}

TEST_F(UtilsTest, testHumanReadableFractionalMegabytes)
{
    ASSERT_EQ(click::Formatter::human_readable_filesize(42*1024*1024+512*1024+512), "42.5 MiB");
}

class TestableDate : public click::Date
{
public:
    using click::Date::timestamp;
};

TEST_F(UtilsTest, testDateParseIso8601)
{
    TestableDate d;
    d.parse_iso8601("1975-11-30T00:00:00Z");
    ASSERT_EQ(186537600, d.timestamp);
}

TEST_F(UtilsTest, testDateFormatted)
{
    TestableDate d;
    d.timestamp = 186537600;
    ASSERT_EQ("Nov 30, 1975", d.formatted());
}

TEST_F(UtilsTest, testAFewDatesFormatted)
{
    TestableDate d;
    d.parse_iso8601("2008-12-06T00:00:00Z");
    ASSERT_EQ("Dec 6, 2008", d.formatted());
    d.parse_iso8601("2013-12-16T00:00:00Z");
    ASSERT_EQ("Dec 16, 2013", d.formatted());
}

TEST_F(UtilsTest, testDateComparesOk)
{
    TestableDate d1;
    d1.parse_iso8601("1975-11-30T00:00:00Z");

    TestableDate d2;
    d2.timestamp = 186537600;

    ASSERT_EQ(d1, d2);
}
