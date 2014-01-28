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

#include "config.h"

#if UNITY_SCOPES_API_HEADERS_NOW_UNDER_UNITY
#include <unity/scopes/ScopeBase.h>
#include <unity/scopes/QueryBase.h>
#else
#include <scopes/ScopeBase.h>
#include <scopes/QueryBase.h>
#endif

#if UNITY_SCOPES_API_NEW_SHORTER_NAMESPACE
namespace scopes = unity::scopes;
#else
namespace scopes = unity::api::scopes;
#endif

namespace click
{
class Scope : public scopes::ScopeBase
{
public:
    virtual int start(std::string const&, scopes::RegistryProxy const&) override;

    virtual void stop() override;

    virtual scopes::QueryBase::UPtr create_query(std::string const& q, scopes::VariantMap const&) override;
};
}
#endif // CLICK_SCOPE_H
