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

#ifndef CLICK_SCOPE_H
#define CLICK_SCOPE_H

#include <memory>
#include <click/network_access_manager.h>
#include <click/webclient.h>

#include <unity/scopes/ScopeBase.h>
#include <unity/scopes/QueryBase.h>
#include <unity/scopes/ActivationQueryBase.h>

#include <click/index.h>

namespace scopes = unity::scopes;

namespace click
{

class DepartmentLookup;

class Scope : public scopes::ScopeBase
{
public:
    Scope();
    ~Scope();

    virtual int start(std::string const&, scopes::RegistryProxy const&) override;

    virtual void run() override;
    virtual void stop() override;

    virtual scopes::SearchQueryBase::UPtr search(scopes::CannedQuery const& q, scopes::SearchMetadata const&) override;
    unity::scopes::PreviewQueryBase::UPtr preview(const unity::scopes::Result&,
            const unity::scopes::ActionMetadata&) override;

    virtual unity::scopes::ActivationQueryBase::UPtr perform_action(unity::scopes::Result const& result, unity::scopes::ActionMetadata const& metadata, std::string const& widget_id, std::string const& action_id) override;

private:
    QSharedPointer<click::network::AccessManager> nam;
    QSharedPointer<click::web::Client> client;
    QSharedPointer<click::Index> index;
    std::shared_ptr<click::DepartmentLookup> depts;

    std::string installApplication(unity::scopes::Result const& result);
};
}
#endif // CLICK_SCOPE_H
