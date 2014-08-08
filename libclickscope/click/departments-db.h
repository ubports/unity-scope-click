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

#ifndef CLICK_DEPARTMENTS_DB_H
#define CLICK_DEPARTMENTS_DB_H

#include <click/departments.h>
#include <string>
#include <set>
#include <unordered_set>
#include <list>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <memory>

class QSqlError;

namespace click
{

class DepartmentsDb
{
public:
    struct DepartmentInfo
    {
        DepartmentInfo(const std::string &id, bool children): id(id), has_children(children) {}
        std::string id;
        bool has_children;

        bool operator==(const DepartmentInfo& other) const
        {
            return id == other.id && has_children == other.has_children;
        }
    };

    DepartmentsDb(const std::string& name, bool create = true);
    DepartmentsDb(const DepartmentsDb& other) = delete;
    DepartmentsDb& operator=(const DepartmentsDb&) = delete;
    virtual ~DepartmentsDb();

    virtual std::string get_department_name(const std::string& department_id, const std::list<std::string>& locales);
    virtual std::unordered_set<std::string> get_packages_for_department(const std::string& department_id, bool recursive = true);
    virtual std::string get_department_for_package(const std::string& package_id);
    virtual bool is_empty(const std::string& department_id);
    virtual bool has_package(const std::string& package_id);
    virtual std::string get_parent_department_id(const std::string& department_id);
    virtual std::list<DepartmentInfo> get_children_departments(const std::string& department_id);
    virtual bool is_descendant_of_department(const std::string& department_id, const std::string& parent_department_id);

    virtual void store_package_mapping(const std::string& package_id, const std::string& department_id);
    virtual void store_department_mapping(const std::string& department_id, const std::string& parent_department_id);
    virtual void store_department_name(const std::string& department_id, const std::string& locale, const std::string& name);

    // these methods are mostly for tests
    virtual int department_mapping_count() const;
    virtual int package_count() const;
    virtual int department_name_count() const;

    virtual void store_departments(const click::DepartmentList& depts, const std::string& locale);

    static std::unique_ptr<DepartmentsDb> open(bool create = true);

protected:
    void init_db();
    void store_departments_(const click::DepartmentList& depts, const std::string& locale);
    static void report_db_error(const QSqlError& error, const std::string& message);

    QSqlDatabase db_;
    std::unique_ptr<QSqlQuery> delete_pkgmap_query_;
    std::unique_ptr<QSqlQuery> delete_depts_query_;
    std::unique_ptr<QSqlQuery> delete_deptnames_query_;
    std::unique_ptr<QSqlQuery> insert_pkgmap_query_;
    std::unique_ptr<QSqlQuery> insert_dept_id_query_;
    std::unique_ptr<QSqlQuery> insert_dept_name_query_;
    std::unique_ptr<QSqlQuery> select_pkgs_by_dept_;
    std::unique_ptr<QSqlQuery> select_dept_for_pkg_;
    std::unique_ptr<QSqlQuery> select_pkg_by_pkgid_;
    std::unique_ptr<QSqlQuery> select_pkgs_by_dept_recursive_;
    std::unique_ptr<QSqlQuery> select_pkgs_count_in_dept_recursive_;
    std::unique_ptr<QSqlQuery> select_parent_dept_;
    std::unique_ptr<QSqlQuery> select_children_depts_;
    std::unique_ptr<QSqlQuery> select_dept_name_;
    std::unique_ptr<QSqlQuery> select_is_descendant_of_dept_;
};

}

#endif
