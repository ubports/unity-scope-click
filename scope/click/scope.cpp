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

#include "qtbridge.h"
#include "scope.h"
#include "query.h"
#include "preview.h"
#include "webclient.h"
#include "network_access_manager.h"
#include <QDebug>

#include <QSharedPointer>

class ScopeActivation : public unity::scopes::ActivationBase
{
    unity::scopes::ActivationResponse activate() override
    {
        return unity::scopes::ActivationResponse(unity::scopes::ActivationResponse::Status::ShowPreview);
    }
};

click::Scope::Scope()
{
    QSharedPointer<click::network::AccessManager> namPtr(
                new click::network::AccessManager());
    QSharedPointer<click::web::Service> servicePtr(
                new click::web::Service(click::SEARCH_BASE_URL, namPtr));
    index = QSharedPointer<click::Index>(new click::Index(servicePtr));
}

click::Scope::~Scope()
{
}

int click::Scope::start(std::string const&, scopes::RegistryProxy const&)
{
    return VERSION;
}

void click::Scope::run()
{
    static const int zero = 0;
    auto emptyCb = []()
    {
    };

    qt::core::world::build_and_run(zero, nullptr, emptyCb);
}

void click::Scope::stop()
{
    qt::core::world::destroy();
}

scopes::QueryBase::UPtr click::Scope::create_query(unity::scopes::Query const& q, scopes::SearchMetadata const&)
{
    return scopes::QueryBase::UPtr(new click::Query(q.query_string()));
}


unity::scopes::QueryBase::UPtr click::Scope::preview(const unity::scopes::Result& result,
        const unity::scopes::ActionMetadata&) {
    scopes::QueryBase::UPtr previewResult(new Preview(result.uri(), index, result));
    return previewResult;
}

unity::scopes::ActivationBase::UPtr click::Scope::perform_action(unity::scopes::Result const& /*result*/, unity::scopes::ActionMetadata const& /* metadata */, std::string const& /* widget_id */, std::string const& action_id)
{
    return scopes::ActivationBase::UPtr(new ScopeActivation());
}

#define EXPORT __attribute__ ((visibility ("default")))

extern "C"
{

    EXPORT
    unity::scopes::ScopeBase*
    // cppcheck-suppress unusedFunction
    UNITY_SCOPE_CREATE_FUNCTION()
    {
        return new click::Scope();
    }

    EXPORT
    void
    // cppcheck-suppress unusedFunction
    UNITY_SCOPE_DESTROY_FUNCTION(unity::scopes::ScopeBase* scope_base)
    {
        delete scope_base;
    }

}
