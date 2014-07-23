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

#include <click/departments-db.h>

#include <memory>
#include <algorithm>
#include <unistd.h>

using namespace click;


class DepartmentsDbConcurrencyTest: public ::testing::Test
{
public:
    const std::string db_path = TEST_DIR "/departments-db-test2.sqlite";

    void TearDown() override
    {
        unlink(db_path.c_str());
    }
};

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
