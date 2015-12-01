#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <click/department-lookup.h>
#include <click/highlights.h>
#include "click/index.h"
#include <click/interface.h>
#include <click/package.h>
#include <gtest/gtest.h>

namespace click
{
namespace test
{
namespace helpers
{
static const std::string FAKE_QUERY {"FAKE_QUERY"};
static const std::string FAKE_CATEGORY_TEMPLATE {"{}"};

// this matcher expects a list of department ids in depts:
// first on the list is the root, followed by children ids.
// the arg of the matcher is unity::scopes::Department ptr.
MATCHER_P(MatchesDepartments, depts, "") {
    auto it = depts.begin();
    if (arg->id() != *it)
        return false;
    auto const subdeps = arg->subdepartments();
    if (subdeps.size() != depts.size() - 1)
        return false;
    for (auto const& sub: subdeps)
    {
        if (sub->id() != *(++it))
            return false;
    }
    return true;
}

class MockIndex : public click::Index {
    click::Packages packages;
    click::Packages recommends;
    click::DepartmentList departments;
    click::DepartmentList bootstrap_departments;
    click::HighlightList bootstrap_highlights;
public:
    MockIndex(click::Packages packages = click::Packages(),
            click::DepartmentList departments = click::DepartmentList(),
            click::DepartmentList boot_departments = click::DepartmentList())
        : Index(QSharedPointer<click::web::Client>()),
          packages(packages),
          departments(departments),
          bootstrap_departments(boot_departments)
    {

    }

    click::web::Cancellable search(const std::string &query, const std::string &department, std::function<void (click::Packages, click::Packages)> callback) override
    {
        do_search(query, department, callback);
        callback(packages, recommends);
        return click::web::Cancellable();
    }

    click::web::Cancellable bootstrap(std::function<void(const click::DepartmentList&, const click::HighlightList&, Error, int)> callback) override
    {
        callback(bootstrap_departments, bootstrap_highlights, click::Index::Error::NoError, 0);
        return click::web::Cancellable();
    }

    MOCK_METHOD3(do_search,
                 void(const std::string&, const std::string&,
                      std::function<void(click::Packages, click::Packages)>));
};

class MockDepartmentsDb : public click::DepartmentsDb
{
public:
    MockDepartmentsDb(const std::string& name, bool create)
        : click::DepartmentsDb(name, create)
    {
    }

    MOCK_METHOD2(get_department_name, std::string(const std::string&, const std::list<std::string>&));
    MOCK_METHOD2(get_packages_for_department, std::unordered_set<std::string>(const std::string&, bool));
    MOCK_METHOD1(get_parent_department_id, std::string(const std::string&));
    MOCK_METHOD1(get_children_departments, std::list<click::DepartmentsDb::DepartmentInfo>(const std::string&));
    MOCK_METHOD1(is_empty, bool(const std::string&));
    MOCK_METHOD2(is_descendant_of_department, bool(const std::string&, const std::string&));

    MOCK_METHOD2(store_package_mapping, void(const std::string&, const std::string&));
    MOCK_METHOD2(store_department_mapping, void(const std::string&, const std::string&));
    MOCK_METHOD3(store_department_name, void(const std::string&, const std::string&, const std::string&));
};

class FakeCategory : public scopes::Category
{
public:
    FakeCategory(std::string const& id, std::string const& title,
                 std::string const& icon, scopes::CategoryRenderer const& renderer) :
       scopes::Category(id, title, icon, renderer)
    {
    }

};
} // namespace helpers
} // namespace test
} // namespace click

#endif // TEST_HELPERS_H
