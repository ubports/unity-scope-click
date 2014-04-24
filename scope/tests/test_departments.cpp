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
#include "fake_json.h"
#include "click/departments.h"
#include "click/department-lookup.h"

class DepartmentsTest : public ::testing::Test
{
    protected:
        void SetUp() override
        {
        }
};

TEST_F(DepartmentsTest, testParsing)
{
    const std::string jsonstr(FAKE_JSON_DEPARTMENTS_ONLY);
    auto depts = click::Department::from_json(jsonstr);
    EXPECT_EQ(3u, depts.size());
    auto it = depts.cbegin();
    {
        auto dep = *it;
        EXPECT_EQ("Games", dep->id());
        EXPECT_EQ("Games", dep->name());
        EXPECT_FALSE(dep->has_children_flag());
        auto subdepts = dep->sub_departments();
        EXPECT_EQ(1u, subdepts.size());
        auto sit = subdepts.cbegin();
        EXPECT_EQ("Board Games", (*sit)->name());
    }
    {
        ++it;
        auto dep = *it;
        EXPECT_EQ("Graphics", dep->id());
        EXPECT_EQ("Graphics", dep->name());
        EXPECT_FALSE(dep->has_children_flag());
        auto subdepts = dep->sub_departments();
        EXPECT_EQ(1u, subdepts.size());
        auto sit = subdepts.cbegin();
        EXPECT_EQ("Drawing", (*sit)->name());
    }
    {
        ++it;
        auto dep = *it;
        EXPECT_EQ("Internet", dep->id());
        EXPECT_EQ("Internet", dep->name());
        EXPECT_FALSE(dep->has_children_flag());
        auto subdepts = dep->sub_departments();
        EXPECT_EQ(3u, subdepts.size());
        auto sit = subdepts.cbegin();
        auto subdep = *sit;
        EXPECT_EQ("Chat", subdep->name());
        subdep = *(++sit);
        EXPECT_EQ("Mail", subdep->name());
        subdep = *(++sit);
        EXPECT_EQ("Web Browsers", subdep->name());
    }
}

TEST_F(DepartmentsTest, testLookup)
{
    auto dep_games = std::make_shared<click::Department>("games", "Games");
    auto dep_rpg = std::make_shared<click::Department>("rpg", "RPG");
    auto dep_strategy = std::make_shared<click::Department>("strategy", "Strategy");
    const std::list<click::Department::SPtr> departments {dep_rpg, dep_strategy};
    dep_games->set_subdepartments(departments);

    const std::list<click::Department::SPtr> root {dep_games};
    click::DepartmentLookup lut;
    lut.rebuild(root);

    EXPECT_EQ(2u, lut.size());
    EXPECT_EQ("games", lut.get_parent("strategy")->id());
    EXPECT_EQ("games", lut.get_parent("rpg")->id());
    EXPECT_EQ(nullptr, lut.get_parent("games"));

    lut.rebuild(root);
    EXPECT_EQ(2u, lut.size());
}

