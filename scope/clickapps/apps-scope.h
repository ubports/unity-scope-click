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

#ifndef APPS_SCOPE_H
#define APPS_SCOPE_H

//#include <click/index.h>
#include <unity/scopes/ScopeBase.h>
#include <unity/scopes/QueryBase.h>
#include <unity/scopes/ActivationQueryBase.h>

#include <future>

namespace scopes = unity::scopes;

namespace click
{

class DepartmentsDb;

class Scope : public scopes::ScopeBase
{
public:
    Scope();
    ~Scope();

    virtual void start(std::string const&) override;

    virtual void run() override;
    virtual void stop() override;

    virtual scopes::SearchQueryBase::UPtr search(scopes::CannedQuery const& q, scopes::SearchMetadata const& metadata) override;
    unity::scopes::PreviewQueryBase::UPtr preview(const unity::scopes::Result&,
            const unity::scopes::ActionMetadata& hints) override;

    virtual unity::scopes::ActivationQueryBase::UPtr perform_action(unity::scopes::Result const& result, unity::scopes::ActionMetadata const& metadata, std::string const& widget_id, std::string const& action_id) override;

private:
    std::promise<void> qt_ready_for_search_p;
    std::future<void> qt_ready_for_search_f;
    std::promise<void> qt_ready_for_preview_p;
    std::future<void> qt_ready_for_preview_f;
    //QSharedPointer<click::Index> index;
    std::shared_ptr<click::DepartmentsDb> depts_db;
};
}
#endif // CLICK_SCOPE_H
