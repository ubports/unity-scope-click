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

#include "scope_activation.h"
#include <click/package.h>
#include <click/interface.h>
#include <click/qtbridge.h>
#include <unity/scopes/ActivationResponse.h>

#include <QDebug>

click::ScopeActivation::ScopeActivation(const unity::scopes::Result& result, const unity::scopes::ActionMetadata& metadata)
    : unity::scopes::ActivationQueryBase(result, metadata)
{
}

unity::scopes::ActivationResponse click::ScopeActivation::activate()
{
    auto response = unity::scopes::ActivationResponse(status_);
    response.set_scope_data(unity::scopes::Variant(hints_));
    return response;
}

void click::ScopeActivation::setStatus(unity::scopes::ActivationResponse::Status status)
{
    status_ = status;
}

void click::ScopeActivation::setHint(std::string key, unity::scopes::Variant value)
{
    hints_[key] = value;
}

click::PerformUninstallAction::PerformUninstallAction(const unity::scopes::Result& result, const unity::scopes::ActionMetadata& metadata)
    : unity::scopes::ActivationQueryBase(result, metadata)
{
}

unity::scopes::ActivationResponse click::PerformUninstallAction::activate()
{
    std::promise<bool> uninstall_success_p;
    std::future<bool> uninstall_success_f = uninstall_success_p.get_future();

    auto const res = result();
    click::Package package;
    package.title = res.title();
    package.name = res["name"].get_string();
    package.version = res["version"].get_string();
    qt::core::world::enter_with_task([this, package, &uninstall_success_p] ()
    {
        click::PackageManager manager;
        manager.uninstall(package, [&](int code, std::string stderr_content) {
                if (code != 0) {
                    qDebug() << "Error removing package:" << stderr_content.c_str();
                    uninstall_success_p.set_value(false);
                } else {
                    qDebug() << "successfully removed package";
                    uninstall_success_p.set_value(true);
                }
            } );
    });

    if (uninstall_success_f.get())
    {
        if (res.contains("lonely_result") && res.value("lonely_result").get_bool())
        {
            return unity::scopes::ActivationResponse(unity::scopes::CannedQuery(APPS_SCOPE_ID.toUtf8().data()));
        }
    }
    return unity::scopes::ActivationResponse(unity::scopes::ActivationResponse::ShowDash);
}
