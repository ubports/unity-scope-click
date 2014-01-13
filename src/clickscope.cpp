#include "clickscope.h"
#include "clickquery.h"


int ClickScope::start(std::string const&, unity::api::scopes::RegistryProxy const&)
{
    return VERSION;
}

void ClickScope::stop()
{
}

unity::api::scopes::QueryBase::UPtr create_query(std::string const& q,
                                                 unity::api::scopes::VariantMap const&)
{
    unity::api::scopes::QueryBase::UPtr query(new ClickQuery(q));
    return query;
}
