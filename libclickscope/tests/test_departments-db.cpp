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
#include <click/departments-db.h>
#include <memory>
#include <algorithm>
#include <unistd.h>

using namespace click;

class DepartmentsDbCheck : public DepartmentsDb
{
public:
    DepartmentsDbCheck(const std::string& name, bool create)
        : DepartmentsDb(name, create)
    {
    }

    void check_sql_queries_finished()
    {
        EXPECT_FALSE(delete_pkgmap_query_->isActive());
        EXPECT_FALSE(delete_depts_query_->isActive());
        EXPECT_FALSE(delete_deptnames_query_->isActive());
        EXPECT_FALSE(insert_pkgmap_query_->isActive());
        EXPECT_FALSE(insert_dept_id_query_->isActive());
        EXPECT_FALSE(insert_dept_name_query_->isActive());
        EXPECT_FALSE(select_pkgs_by_dept_->isActive());
        EXPECT_FALSE(select_pkgs_by_dept_recursive_->isActive());
        EXPECT_FALSE(select_parent_dept_->isActive());
        EXPECT_FALSE(select_children_depts_->isActive());
        EXPECT_FALSE(select_dept_name_->isActive());
    }
};

class DepartmentsDbTest: public ::testing::Test
{
public:
    const std::string db_path = ":memory:";

    void SetUp() override
    {
        db.reset(new DepartmentsDbCheck(db_path, true));
        db->store_department_name("tools", "", "Tools");
        db->store_department_name("office", "", "Office");

        db->store_department_mapping("office", "tools");

        db->store_package_mapping("app1", "tools");
        db->store_package_mapping("app2", "office");

        db->store_department_name("games", "", "Games");
        db->store_department_name("games", "pl_PL", "Gry");
        db->store_department_name("rpg", "", "RPG");
        db->store_department_name("card", "", "Card");
        db->store_department_name("fps", "", "First Person Shooter");

        db->store_department_mapping("rpg", "games");
        db->store_department_mapping("card", "games");
        db->store_department_mapping("fps", "games");

        db->store_package_mapping("game1", "rpg");
        db->store_package_mapping("game2", "fps");
    }

protected:
    std::unique_ptr<DepartmentsDbCheck> db;
};

class DepartmentsDbConcurrencyTest: public ::testing::Test
{
public:
    const std::string db_path = TEST_DIR "/departments-db-test2.sqlite";

    void TearDown() override
    {
        unlink(db_path.c_str());
    }
};

TEST_F(DepartmentsDbTest, testDepartmentNameLookup)
{
    {
        EXPECT_EQ("Games", db->get_department_name("games", {"en_EN", ""}));
        EXPECT_EQ("Gry", db->get_department_name("games", {"pl_PL", ""}));
        EXPECT_EQ("First Person Shooter", db->get_department_name("fps", {"en_EN", ""}));
        EXPECT_EQ("Office", db->get_department_name("office", {"en_EN", ""}));
        EXPECT_EQ("Tools", db->get_department_name("tools", {"en_EN", ""}));

        EXPECT_THROW(db->get_department_name("xyz", {"en_EN", ""}), std::logic_error);
    }
}

TEST_F(DepartmentsDbTest, testDepartmentNameUpdates)
{
    {
        EXPECT_EQ(7u, db->department_name_count());
        db->store_department_name("tools", "", "Tools!");
        EXPECT_EQ("Tools!", db->get_department_name("tools", {"en_EN", ""}));
        db->store_department_name("games", "pl_PL", "Gry!!!");
        EXPECT_EQ("Gry!!!", db->get_department_name("games", {"pl_PL"}));
        EXPECT_EQ(7u, db->department_name_count());
    }
}

TEST_F(DepartmentsDbTest, testDepartmentParentLookup)
{
    {
        EXPECT_EQ("games", db->get_parent_department_id("rpg"));
        EXPECT_EQ("games", db->get_parent_department_id("card"));
        EXPECT_EQ("", db->get_parent_department_id("games"));

        EXPECT_EQ("tools", db->get_parent_department_id("office"));
        EXPECT_EQ("", db->get_parent_department_id("tools"));

        EXPECT_THROW(db->get_parent_department_id("xyz"), std::logic_error);
    }
}

TEST_F(DepartmentsDbTest, testDepartmentChildrenLookup)
{
    {
        EXPECT_EQ(0, db->get_children_departments("xyz").size());
    }
    {
        auto depts = db->get_children_departments("");
        EXPECT_EQ(2u, depts.size());
        EXPECT_TRUE(std::find(depts.begin(), depts.end(), DepartmentsDb::DepartmentInfo("tools", true)) != depts.end());
        EXPECT_TRUE(std::find(depts.begin(), depts.end(), DepartmentsDb::DepartmentInfo("games", true)) != depts.end());
    }
    {
        auto depts = db->get_children_departments("tools");
        EXPECT_EQ(1u, depts.size());
        EXPECT_TRUE(std::find(depts.begin(), depts.end(), DepartmentsDb::DepartmentInfo("office", false)) != depts.end());
    }
    {
        auto depts = db->get_children_departments("games");
        EXPECT_EQ(3u, depts.size());
        EXPECT_TRUE(std::find(depts.begin(), depts.end(), DepartmentsDb::DepartmentInfo("rpg", false)) != depts.end());
        EXPECT_TRUE(std::find(depts.begin(), depts.end(), DepartmentsDb::DepartmentInfo("fps", false)) != depts.end());
        EXPECT_TRUE(std::find(depts.begin(), depts.end(), DepartmentsDb::DepartmentInfo("card", false)) != depts.end());
    }
}

TEST_F(DepartmentsDbTest, testPackageLookup)
{
    {
        auto pkgs = db->get_packages_for_department("rpg", false);
        EXPECT_EQ(1u, pkgs.size());
        EXPECT_TRUE(pkgs.find("game1") != pkgs.end());
    }
    {
        auto pkgs = db->get_packages_for_department("fps", false);
        EXPECT_EQ(1u, pkgs.size());
        EXPECT_TRUE(pkgs.find("game2") != pkgs.end());
    }
    {
        auto pkgs = db->get_packages_for_department("card", false);
        EXPECT_EQ(0, pkgs.size());
    }
}

TEST_F(DepartmentsDbTest, testRecursivePackageLookup)
{
    {
        // get packages from subdepartments of games department
        auto pkgs = db->get_packages_for_department("games", true);
        EXPECT_EQ(2u, pkgs.size());
        EXPECT_TRUE(pkgs.find("game1") != pkgs.end());
        EXPECT_TRUE(pkgs.find("game2") != pkgs.end());
    }
    {
        // rpg has no subdepartments, so we just get the packages of rpg department
        auto pkgs = db->get_packages_for_department("rpg", true);
        EXPECT_EQ(1u, pkgs.size());
        EXPECT_TRUE(pkgs.find("game1") != pkgs.end());
    }
    {
        auto pkgs = db->get_packages_for_department("card", true);
        EXPECT_EQ(0, pkgs.size());
    }
}

TEST_F(DepartmentsDbTest, testPackageUpdates)
{
    auto pkgs = db->get_packages_for_department("fps");
    EXPECT_EQ(1, pkgs.size());
    EXPECT_TRUE(pkgs.find("game2") != pkgs.end());

    // game2 gets moved to card and removed from fps
    db->store_package_mapping("game2", "card");

    pkgs = db->get_packages_for_department("fps", false);
    EXPECT_EQ(0, pkgs.size());
    EXPECT_TRUE(pkgs.find("game2") == pkgs.end());
    pkgs = db->get_packages_for_department("card", false);
    EXPECT_EQ(1, pkgs.size());
    EXPECT_TRUE(pkgs.find("game2") != pkgs.end());
}

TEST_F(DepartmentsDbTest, testInvalidDepartments)
{
    EXPECT_THROW(db->get_parent_department_id(""), std::logic_error);
    EXPECT_THROW(db->get_parent_department_id("foooo"), std::logic_error);
}

TEST_F(DepartmentsDbTest, testEmptyArguments)
{
    EXPECT_THROW(db->store_department_name("", "", "Foo"), std::logic_error);
    EXPECT_THROW(db->store_department_name("foo", "", ""), std::logic_error);
    EXPECT_THROW(db->store_department_mapping("", "foo"), std::logic_error);
    EXPECT_THROW(db->store_department_mapping("foo", ""), std::logic_error);
    EXPECT_THROW(db->store_package_mapping("", "foo"), std::logic_error);
    EXPECT_THROW(db->store_package_mapping("foo", ""), std::logic_error);
}

TEST_F(DepartmentsDbTest, testNoDuplicates)
{
    db->store_package_mapping("game2", "fps");
    db->store_package_mapping("game2", "fps");
    db->store_department_name("games", "pl_PL", "Gry");
    db->store_department_name("games", "pl_PL", "Gry");
    db->store_department_mapping("office", "tools");
    db->store_department_mapping("office", "tools");

    EXPECT_EQ(7u, db->department_name_count());
    EXPECT_EQ(4u, db->package_count());
}

TEST_F(DepartmentsDbTest, testSqlQueriesFinished)
{
    db->get_department_name("games", {"en_EN", ""});
    db->check_sql_queries_finished();

    db->get_parent_department_id("rpg");
    db->check_sql_queries_finished();

    db->get_packages_for_department("rpg", false);
    db->check_sql_queries_finished();

    db->get_packages_for_department("rpg", true);
    db->check_sql_queries_finished();

    db->get_children_departments("games");
    db->check_sql_queries_finished();

    db->department_name_count();
    db->check_sql_queries_finished();

    db->department_mapping_count();
    db->check_sql_queries_finished();

    db->package_count();
    db->check_sql_queries_finished();

    db->store_package_mapping("game2", "fps");
    db->check_sql_queries_finished();

    db->store_department_name("games", "pl_PL", "Gry");
    db->check_sql_queries_finished();

    db->store_department_mapping("office", "tools");
    db->check_sql_queries_finished();
}

// Keep the numbers below at a reasonable level; it takes around 20 seconds
// to run this test on a i7-2620M with the default values.
const int NUM_OF_WRITE_OPS = 50;
const int NUM_OF_READ_OPS = 100;

void populate_departments(DepartmentsDb *db, int repeat_count)
{
    click::DepartmentList depts;
    // generate departments with one subdepartment
    for (int i = 0; i < 20; i++)
    {
        auto const id = std::to_string(i);
        auto parent = std::make_shared<click::Department>(id, "Department " + id, "href", true);
        parent->set_subdepartments({std::make_shared<click::Department>(id + "sub", "Subdepartment of " + id, "href", false)});
        depts.push_back(parent);
    }

    for (int i = 0; i < repeat_count; i++)
    {
        ASSERT_NO_THROW(db->store_departments(depts, ""));

        // generate apps
        for (int j = 0; j < 50; j++)
        {
            auto const id = std::to_string(j);
            ASSERT_NO_THROW(db->store_package_mapping("app" + id, id));
        }
    }
}

TEST_F(DepartmentsDbConcurrencyTest, ConcurrentReadWrite)
{
    // populate the db initially to make sure reader doesn't fail if it's faster than writer
    {
        DepartmentsDb db(db_path, true);
        populate_departments(&db, 1);
    }

    pid_t writer_pid = fork();
    if (writer_pid < 0)
    {
        FAIL();
    }
    else if (writer_pid == 0) // writer child process
    {
        SCOPED_TRACE("writer");
        std::unique_ptr<DepartmentsDb> db;
        ASSERT_NO_THROW(db.reset(new DepartmentsDb(db_path, true)));

        populate_departments(db.get(), NUM_OF_WRITE_OPS);

        exit(0);
    }
    else // parent process
    {
        pid_t reader_pid = fork();
        if (reader_pid < 0)
        {
            FAIL();
        }
        else if (reader_pid == 0) // reader child process
        {
            SCOPED_TRACE("reader");
            std::unique_ptr<DepartmentsDb> db;
            ASSERT_NO_THROW(db.reset(new DepartmentsDb(db_path, false)));

            for (int i = 0; i < NUM_OF_READ_OPS; i++)
            {
                ASSERT_NO_THROW(db->get_department_name("1", {""}));
                ASSERT_NO_THROW(db->get_parent_department_id("1"));
                ASSERT_NO_THROW(db->get_packages_for_department("1", false));
                ASSERT_NO_THROW(db->get_packages_for_department("1", true));
                ASSERT_NO_THROW(db->get_children_departments(""));
                ASSERT_NO_THROW(db->department_name_count());
                ASSERT_NO_THROW(db->department_mapping_count());
                ASSERT_NO_THROW(db->package_count());
            }
            exit(0);
        }
        else // parent process
        {
            wait(nullptr);
            wait(nullptr);
        }
    }
}

TEST(DepartmentsDb, testOpenFailsOnUnitializedDb)
{
    ASSERT_THROW(
            {
                DepartmentsDb db(":memory:", false);
            }, std::runtime_error);
}
