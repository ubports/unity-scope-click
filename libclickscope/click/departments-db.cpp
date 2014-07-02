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

std::unique_ptr<click::DepartmentsDb> DepartmentsDb::create_db()
{
    auto const path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (!path.isEmpty())
    {
        QDir("/").mkpath(path);
        const std::string dbpath = path.toStdString() + "/click-departments.db";
        return std::unique_ptr<DepartmentsDb>(new DepartmentsDb(dbpath));
    }
    throw std::runtime_error("Cannot determine cache directory");
}

DepartmentsDb::DepartmentsDb(const std::string& name)
{
    init_db(name);

    delete_pkgmap_query_.reset(new QSqlQuery(db_));
    insert_pkgmap_query_.reset(new QSqlQuery(db_));
    insert_dept_id_query_.reset(new QSqlQuery(db_));
    insert_dept_name_query_.reset(new QSqlQuery(db_));
    select_pkgs_by_dept_.reset(new QSqlQuery(db_));
    select_pkgs_by_dept_recursive_.reset(new QSqlQuery(db_));
    select_parent_dept_.reset(new QSqlQuery(db_));
    select_children_depts_.reset(new QSqlQuery(db_));
    select_dept_name_.reset(new QSqlQuery(db_));

    delete_pkgmap_query_->prepare("DELETE FROM pkgmap WHERE pkgid=:pkgid");
    insert_pkgmap_query_->prepare("INSERT OR REPLACE INTO pkgmap (pkgid, deptid) VALUES (:pkgid, :deptid)");
    insert_dept_id_query_->prepare("INSERT OR REPLACE INTO depts (deptid, parentid) VALUES (:deptid, :parentid)");
    insert_dept_name_query_->prepare("INSERT OR REPLACE INTO deptnames (deptid, locale, name) VALUES (:deptid, :locale, :name)");
    select_pkgs_by_dept_->prepare("SELECT pkgid FROM pkgmap WHERE deptid=:deptid");
    select_pkgs_by_dept_recursive_->prepare("WITH RECURSIVE recdepts(deptid) AS (SELECT deptid FROM depts_v WHERE deptid=:deptid UNION SELECT depts_v.deptid FROM recdepts,depts_v WHERE recdepts.deptid=depts_v.parentid) SELECT pkgid FROM pkgmap NATURAL JOIN recdepts");
    select_children_depts_->prepare("SELECT deptid,(SELECT COUNT(1) from DEPTS_V AS inner WHERE inner.parentid=outer.deptid) FROM DEPTS_V AS outer WHERE parentid=:parentid");
    select_parent_dept_->prepare("SELECT parentid FROM depts_v WHERE deptid=:deptid");
    select_dept_name_->prepare("SELECT name FROM deptnames WHERE deptid=:deptid AND locale=:locale");
}

void DepartmentsDb::init_db(const std::string& name)
{
    db_ = QSqlDatabase::addDatabase("QSQLITE");
    db_.setDatabaseName(QString::fromStdString(name));
    if (!db_.open())
    {
        throw std::runtime_error("Cannot open departments database");
    }

    QSqlQuery query;

    // FIXME: for some reason enabling foreign keys gives errors about number of arguments of prepared queries when doing query.exec(); do not enable
    // them for now.
    // query.exec("PRAGMA foreign_keys = ON");

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
    query.exec("INSERT INTO meta (name, value) VALUES ('version', 1)");

    // view of the depts table that automatically adds fake "" department for root departments
    if (!query.exec("CREATE VIEW IF NOT EXISTS depts_v AS SELECT deptid, parentid FROM depts UNION SELECT deptid,'' AS parentid FROM deptnames WHERE NOT EXISTS "
        "(SELECT * FROM depts WHERE depts.deptid=deptnames.deptid)"))
    {
        report_db_error(query.lastError(), "Failed to create depts_v view");
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
            return select_dept_name_->value(0).toString().toStdString();
        }
    }
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
        throw std::logic_error("Unknown department '" + department_id + "'");
    }
    return select_parent_dept_->value(0).toString().toStdString();
}

std::list<DepartmentsDb::DepartmentInfo> DepartmentsDb::get_children_departments(const std::string& department_id)
{
    // FIXME: this should only return departments that have results, and set 'has_children' flag on the same basis.
    select_children_depts_->bindValue(":parentid", QVariant(QString::fromStdString(department_id)));
    if (!select_children_depts_->exec())
    {
        report_db_error(select_children_depts_->lastError(), "Failed to query for children departments of " + department_id);
    }

    std::list<DepartmentInfo> depts;
    while (select_children_depts_->next())
    {
        const DepartmentInfo inf(select_children_depts_->value(0).toString().toStdString(), select_children_depts_->value(1).toBool());
        depts.push_back(inf);
    }

    return depts;
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
    return pkgs;
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

    if (!db_.commit())
    {
        std::cerr << "Failed to commit transaction" << std::endl;
    }
}

void DepartmentsDb::store_department_mapping(const std::string& department_id, const std::string& parent_department_id)
{
    if (department_id.empty())
    {
        throw std::logic_error("Invalid empty department id");
    }

    if (parent_department_id.empty())
    {
        throw std::logic_error("Invalid empty parent department id");
    }

    insert_dept_id_query_->bindValue(":deptid", QVariant(QString::fromStdString(department_id)));
    insert_dept_id_query_->bindValue(":parentid", QVariant(QString::fromStdString(parent_department_id)));
    if (!insert_dept_id_query_->exec())
    {
        report_db_error(insert_dept_id_query_->lastError(), "Failed to insert into depts");
    }
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

}
