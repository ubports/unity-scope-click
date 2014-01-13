#ifndef CLICKSCOPE_H
#define CLICKSCOPE_H

#include <scopes/ScopeBase.h>
#include <scopes/QueryBase.h>

class ClickScope : public unity::api::scopes::ScopeBase
{
public:
    virtual int start(std::string const&, unity::api::scopes::RegistryProxy const&) override;

    virtual void stop() override;

    virtual unity::api::scopes::QueryBase::UPtr create_query(std::string const& q,
            unity::api::scopes::VariantMap const&) override;
};

#endif
