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

#include "scope.h"
#include "query.h"
#include "preview.h"

#include<QCoreApplication>

int click::Scope::start(std::string const&, scopes::RegistryProxy const&)
{
    return VERSION;
}

void click::Scope::run() {
    int zero = 0;
    QCoreApplication *app = new QCoreApplication(zero, nullptr);
    app->exec();
    delete app; // If exec throws this leaks, but the process will die in milliseconds so we don't care.
}

void click::Scope::stop()
{
    QCoreApplication::instance()->quit();
}

scopes::QueryBase::UPtr click::Scope::create_query(std::string const& q, scopes::VariantMap const&)
{
    return scopes::QueryBase::UPtr(new click::Query(q));
}


unity::scopes::QueryBase::UPtr click::Scope::preview(const unity::scopes::Result& result,
        const unity::scopes::VariantMap&) {
    scopes::QueryBase::UPtr preview(new Preview(result.uri(), result));
    return preview;
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
