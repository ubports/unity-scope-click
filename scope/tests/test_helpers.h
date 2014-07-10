#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <click/department-lookup.h>
#include <click/highlights.h>
#include "click/index.h"
#include <click/interface.h>
#include <click/package.h>

namespace click
{
namespace test
{
namespace helpers
{
static const std::string FAKE_QUERY {"FAKE_QUERY"};
static const std::string FAKE_CATEGORY_TEMPLATE {"{}"};


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

    click::web::Cancellable search(const std::string &query, std::function<void (click::Packages, click::Packages)> callback) override
    {
        do_search(query, callback);
        callback(packages, recommends);
        return click::web::Cancellable();
    }

    click::web::Cancellable bootstrap(std::function<void(const click::DepartmentList&, const click::HighlightList&, Error, int)> callback) override
    {
        callback(bootstrap_departments, bootstrap_highlights, click::Index::Error::NoError, 0);
        return click::web::Cancellable();
    }

    MOCK_METHOD2(do_search,
                 void(const std::string&,
                      std::function<void(click::Packages, click::Packages)>));
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
