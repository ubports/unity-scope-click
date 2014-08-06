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

#include "departments-db.h"
#include <stdexcept>
#include <iostream>
#include <QSqlError>
#include <QSqlRecord>
#include <QVariant>
#include <QStandardPaths>
#include <QDir>

namespace click
{

std::unique_ptr<click::DepartmentsDb> DepartmentsDb::open(bool create)
{
    auto const path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (!path.isEmpty())
    {
        QDir("/").mkpath(path);
        const std::string dbpath = path.toStdString() + "/click-departments.db";
        return std::unique_ptr<DepartmentsDb>(new DepartmentsDb(dbpath, create));
    }
    throw std::runtime_error("Cannot determine cache directory");
}

DepartmentsDb::DepartmentsDb(const std::string& name, bool create)
{
    db_ = QSqlDatabase::addDatabase("QSQLITE");
    db_.setDatabaseName(QString::fromStdString(name));
    if (!db_.open())
    {
        throw std::runtime_error("Cannot open departments database");
    }

    if (create)
    {
        init_db();
    }
    else
    {
        QSqlQuery query;
        // check for existence of meta table to see if we're dealing with uninitialized database
        if (!query.exec("SELECT 1 FROM meta"))
        {
            throw std::runtime_error("Invalid departments database");
        }
    }

    delete_pkgmap_query_.reset(new QSqlQuery(db_));
    delete_depts_query_.reset(new QSqlQuery(db_));
    delete_deptnames_query_.reset(new QSqlQuery(db_));
    insert_pkgmap_query_.reset(new QSqlQuery(db_));
    insert_dept_id_query_.reset(new QSqlQuery(db_));
    insert_dept_name_query_.reset(new QSqlQuery(db_));
    select_pkgs_by_dept_.reset(new QSqlQuery(db_));
    select_pkg_by_pkgid_.reset(new QSqlQuery(db_));
    select_pkgs_by_dept_recursive_.reset(new QSqlQuery(db_));
    select_pkgs_count_in_dept_recursive_.reset(new QSqlQuery(db_));
    select_parent_dept_.reset(new QSqlQuery(db_));
    select_children_depts_.reset(new QSqlQuery(db_));
    select_dept_name_.reset(new QSqlQuery(db_));

    delete_pkgmap_query_->prepare("DELETE FROM pkgmap WHERE pkgid=:pkgid");
    delete_depts_query_->prepare("DELETE FROM depts");
    delete_deptnames_query_->prepare("DELETE FROM deptnames WHERE locale=:locale");
    insert_pkgmap_query_->prepare("INSERT OR REPLACE INTO pkgmap (pkgid, deptid) VALUES (:pkgid, :deptid)");
    insert_dept_id_query_->prepare("INSERT OR REPLACE INTO depts (deptid, parentid) VALUES (:deptid, :parentid)");
    insert_dept_name_query_->prepare("INSERT OR REPLACE INTO deptnames (deptid, locale, name) VALUES (:deptid, :locale, :name)");
    select_pkgs_by_dept_->prepare("SELECT pkgid FROM pkgmap WHERE deptid=:deptid");
    select_pkgs_by_dept_recursive_->prepare("WITH RECURSIVE recdepts(deptid) AS (SELECT deptid FROM depts WHERE deptid=:deptid UNION SELECT depts.deptid FROM recdepts,depts WHERE recdepts.deptid=depts.parentid) SELECT pkgid FROM pkgmap NATURAL JOIN recdepts");
    select_pkgs_count_in_dept_recursive_->prepare("WITH RECURSIVE recdepts(deptid) AS (SELECT deptid FROM depts WHERE deptid=:deptid UNION SELECT depts.deptid FROM recdepts,depts WHERE recdepts.deptid=depts.parentid) SELECT COUNT(pkgid) FROM pkgmap NATURAL JOIN recdepts");
    select_pkg_by_pkgid_->prepare("SELECT pkgid FROM pkgmap WHERE pkgid=:pkgid");
    select_children_depts_->prepare("SELECT deptid,(SELECT COUNT(1) from depts AS inner WHERE inner.parentid=outer.deptid) FROM depts AS outer WHERE parentid=:parentid");
    select_parent_dept_->prepare("SELECT parentid FROM depts WHERE deptid=:deptid");
    select_dept_name_->prepare("SELECT name FROM deptnames WHERE deptid=:deptid AND locale=:locale");
}

DepartmentsDb::~DepartmentsDb()
{
}

void DepartmentsDb::init_db()
{
    //
    // CAUTION:
    // DON'T FORGET TO BUMP SCHEMA VERSION BELOW AND HANDLE SCHEMA UPGRADE IN data/update_schema.sh
    // WHENEVER YOU CHANGE ANY OF THE TABLES BELOW!

    QSqlQuery query;

    // FIXME: for some reason enabling foreign keys gives errors about number of arguments of prepared queries when doing query.exec(); do not enable
    // them for now.
    // query.exec("PRAGMA foreign_keys = ON");

    // activate write-ahead logging, see http://sqlite.org/wal.html to avoid transaction errors with concurrent reads and writes
    query.exec("PRAGMA journal_mode=WAL");

    db_.transaction();

    // package id -> department id mapping table
    if (!query.exec("CREATE TABLE IF NOT EXISTS pkgmap (pkgid TEXT, deptid TEXT, CONSTRAINT pkey PRIMARY KEY (pkgid, deptid))"))
    {
        report_db_error(query.lastError(), "Failed to create pkgmap table");
    }

    // department id -> parent department id mapping table
    if (!query.exec("CREATE TABLE IF NOT EXISTS depts (deptid TEXT, parentid TEXT, CONSTRAINT pkey PRIMARY KEY (deptid, parentid), CONSTRAINT fkey FOREIGN KEY (deptid) REFERENCES deptnames(deptid))"))
    {
        report_db_error(query.lastError(), "Failed to create depts table");
    }

    // department id, locale -> deparment name mapping table
    if (!query.exec("CREATE TABLE IF NOT EXISTS deptnames (deptid TEXT, locale TEXT, name TEXT, CONSTRAINT deptuniq PRIMARY KEY (deptid, locale))"))
    {
        report_db_error(query.lastError(), "Failed to create depts table");
    }

    // name -> value table for storing arbitrary values such as schema version
    if (!query.exec("CREATE TABLE IF NOT EXISTS meta (name TEXT PRIMARY KEY, value TEXT)"))
    {
        report_db_error(query.lastError(), "Failed to create meta table");
    }

    //
    // note: this will fail due to unique constraint, but that's fine; it's expected to succeed only when new database is created; in other
    // cases the version needs to be bumped in the update_schema.sh script.
    query.exec("INSERT INTO meta (name, value) VALUES ('version', 2)");

    if (!db_.commit())
    {
        report_db_error(db_.lastError(), "Failed to commit init transaction");
    }
}

void DepartmentsDb::report_db_error(const QSqlError& error, const std::string& message)
{
    throw std::runtime_error(message + ": " + error.text().toStdString());
}

std::string DepartmentsDb::get_department_name(const std::string& department_id, const std::list<std::string>& locales)
{
    for (auto const& locale: locales)
    {
        select_dept_name_->bindValue(":deptid", QVariant(QString::fromStdString(department_id)));
        select_dept_name_->bindValue(":locale", QVariant(QString::fromStdString(locale)));

        if (!select_dept_name_->exec())
        {
            report_db_error(select_dept_name_->lastError(), "Failed to query for department name of " + department_id + ", locale " + locale);
        }

        if (select_dept_name_->next())
        {
            auto const res = select_dept_name_->value(0).toString().toStdString();
            select_dept_name_->finish();
            return res;
        }
    }
    select_dept_name_->finish();
    throw std::logic_error("No name for department " + department_id);
}

std::string DepartmentsDb::get_parent_department_id(const std::string& department_id)
{
    select_parent_dept_->bindValue(":deptid", QVariant(QString::fromStdString(department_id)));
    if (!select_parent_dept_->exec())
    {
        report_db_error(select_parent_dept_->lastError(), "Failed to query for parent department " + department_id);
    }
    if (!select_parent_dept_->next())
    {
        select_dept_name_->finish();
        throw std::logic_error("Unknown department '" + department_id + "'");
    }
    auto const res = select_parent_dept_->value(0).toString().toStdString();
    select_parent_dept_->finish();
    return res;
}

std::list<DepartmentsDb::DepartmentInfo> DepartmentsDb::get_children_departments(const std::string& department_id)
{
    select_children_depts_->bindValue(":parentid", QVariant(QString::fromStdString(department_id)));
    if (!select_children_depts_->exec())
    {
        report_db_error(select_children_depts_->lastError(), "Failed to query for children departments of " + department_id);
    }

    std::list<DepartmentInfo> depts;
    while (select_children_depts_->next())
    {
        // only return child department if it's not empty
        if (!is_empty(select_children_depts_->value(0).toString().toStdString()))
        {
            const DepartmentInfo inf(select_children_depts_->value(0).toString().toStdString(), select_children_depts_->value(1).toBool());
            depts.push_back(inf);
        }
    }

    select_children_depts_->finish();

    return depts;
}

bool DepartmentsDb::is_empty(const std::string& department_id)
{
    select_pkgs_count_in_dept_recursive_->bindValue(":deptid", QVariant(QString::fromStdString(department_id)));
    if (!select_pkgs_count_in_dept_recursive_->exec() || !select_pkgs_count_in_dept_recursive_->next())
    {
        report_db_error(select_pkgs_count_in_dept_recursive_->lastError(), "Failed to query for package count of department " + department_id);
    }
    auto cnt = select_pkgs_count_in_dept_recursive_->value(0).toInt();
    select_pkgs_count_in_dept_recursive_->finish();
    return cnt == 0;
}

std::unordered_set<std::string> DepartmentsDb::get_packages_for_department(const std::string& department_id, bool recursive)
{
    std::unordered_set<std::string> pkgs;
    QSqlQuery *query = recursive ? select_pkgs_by_dept_recursive_.get() : select_pkgs_by_dept_.get();
    query->bindValue(":deptid", QVariant(QString::fromStdString(department_id)));
    if (!query->exec())
    {
        report_db_error(query->lastError(), "Failed to query for packages of department " + department_id);
    }
    while (query->next())
    {
        pkgs.insert(query->value(0).toString().toStdString());
    }
    query->finish();
    return pkgs;
}

bool DepartmentsDb::has_package(const std::string& package_id)
{
    select_pkg_by_pkgid_->bindValue(":pkgid", QVariant(QString::fromStdString(package_id)));
    if (!select_pkg_by_pkgid_->exec())
    {
        report_db_error(select_parent_dept_->lastError(), "Failed to query for package " + package_id);
    }
    if (!select_pkg_by_pkgid_->next())
    {
        select_pkg_by_pkgid_->finish();
        return false;
    }
    select_pkg_by_pkgid_->finish();
    return true;

}

void DepartmentsDb::store_package_mapping(const std::string& package_id, const std::string& department_id)
{
    if (package_id.empty())
    {
        throw std::logic_error("Invalid empty package_id");
    }

    if (department_id.empty())
    {
        throw std::logic_error("Invalid empty department id");
    }

    if (!db_.transaction())
    {
        std::cerr << "Failed to start transaction" << std::endl;
    }

    // delete package mapping first from any departments
    delete_pkgmap_query_->bindValue(":pkgid", QVariant(QString::fromStdString(package_id)));
    delete_pkgmap_query_->exec();
    delete_pkgmap_query_->finish();

    insert_pkgmap_query_->bindValue(":pkgid", QVariant(QString::fromStdString(package_id)));
    insert_pkgmap_query_->bindValue(":deptid", QVariant(QString::fromStdString(department_id)));
    if (!insert_pkgmap_query_->exec())
    {
        if (!db_.rollback())
        {
            std::cerr << "Failed to rollback transaction" << std::endl;
        }
        report_db_error(insert_pkgmap_query_->lastError(), "Failed to insert into pkgmap");
    }

    insert_pkgmap_query_->finish();

    if (!db_.commit())
    {
        db_.rollback();
        report_db_error(db_.lastError(), "Failed to commit transaction in store_package_mapping");
    }
}

void DepartmentsDb::store_department_mapping(const std::string& department_id, const std::string& parent_department_id)
{
    if (department_id.empty())
    {
        throw std::logic_error("Invalid empty department id");
    }

    insert_dept_id_query_->bindValue(":deptid", QVariant(QString::fromStdString(department_id)));
    insert_dept_id_query_->bindValue(":parentid", QVariant(QString::fromStdString(parent_department_id)));
    if (!insert_dept_id_query_->exec())
    {
        report_db_error(insert_dept_id_query_->lastError(), "Failed to insert into depts");
    }
    insert_dept_id_query_->finish();
}

void DepartmentsDb::store_department_name(const std::string& department_id, const std::string& locale, const std::string& name)
{
    if (department_id.empty())
    {
        throw std::logic_error("Invalid empty department id");
    }

    if (name.empty())
    {
        throw std::logic_error("Invalid empty department name");
    }

    insert_dept_name_query_->bindValue(":deptid", QVariant(QString::fromStdString(department_id)));
    insert_dept_name_query_->bindValue(":locale", QVariant(QString::fromStdString(locale)));
    insert_dept_name_query_->bindValue(":name", QVariant(QString::fromStdString(name)));

    if (!insert_dept_name_query_->exec())
    {
        report_db_error(insert_dept_name_query_->lastError(), "Failed to insert into deptnames");
    }
    insert_dept_name_query_->finish();
}

int DepartmentsDb::department_mapping_count() const
{
    QSqlQuery q(db_);
    if (!q.exec("SELECT COUNT(*) FROM depts") || !q.next())
    {
        report_db_error(q.lastError(), "Failed to query depts table");
    }
    return q.value(0).toInt();
}

int DepartmentsDb::package_count() const
{
    QSqlQuery q(db_);
    if (!q.exec("SELECT COUNT(*) FROM pkgmap") || !q.next())
    {
        report_db_error(q.lastError(), "Failed to query pkgmap table");
    }
    return q.value(0).toInt();
}

int DepartmentsDb::department_name_count() const
{
    QSqlQuery q(db_);
    if (!q.exec("SELECT COUNT(*) FROM deptnames") || !q.next())
    {
        report_db_error(q.lastError(), "Failed to query deptnames table");
    }
    return q.value(0).toInt();
}

void DepartmentsDb::store_departments_(const click::DepartmentList& depts, const std::string& locale)
{
    for (auto const& dept: depts)
    {
        store_department_name(dept->id(), locale, dept->name());
        for (auto const& subdep: dept->sub_departments())
        {
            store_department_mapping(subdep->id(), dept->id());
        }
        store_departments_(dept->sub_departments(), locale);
    }
}

void DepartmentsDb::store_departments(const click::DepartmentList& depts, const std::string& locale)
{
    if (!db_.transaction())
    {
        std::cerr << "Failed to start transaction" << std::endl;
    }

    //
    // delete existing departments for given locale first
    delete_deptnames_query_->bindValue(":locale", QVariant(QString::fromStdString(locale)));
    if (!delete_deptnames_query_->exec())
    {
        db_.rollback();
        report_db_error(delete_deptnames_query_->lastError(), "Failed to delete from deptnames");
    }
    if (!delete_depts_query_->exec())
    {
        db_.rollback();
        report_db_error(delete_depts_query_->lastError(), "Failed to delete from depts");
    }

    delete_deptnames_query_->finish();
    delete_depts_query_->finish();

    // store mapping of top level departments to root ""
    for (auto const& dept: depts)
    {
        store_department_mapping(dept->id(), "");
    }

    store_departments_(depts, locale);

    if (!db_.commit())
    {
        db_.rollback();
        report_db_error(db_.lastError(), "Failed to commit transaction in store_departments");
    }
}

}
