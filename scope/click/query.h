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

#ifndef CLICK_QUERY_H
#define CLICK_QUERY_H

#include "config.h"

#if UNITY_SCOPES_API_HEADERS_NOW_UNDER_UNITY
#include <unity/scopes/SearchQuery.h>
#else 
#include <scopes/SearchQuery.h>
#endif

#if UNITY_SCOPES_API_NEW_SHORTER_NAMESPACE
namespace scopes = unity::scopes;
#else
namespace scopes = unity::api::scopes;
#endif

#include <QScopedPointer>

namespace click
{
class Query : public scopes::SearchQuery
{
public:
    Query(std::string const& query);
    ~Query();

    virtual void cancelled() override;

    virtual void run(scopes::SearchReplyProxy const& reply) override;

private:
    struct Private;
    QScopedPointer<Private> impl;
};
}

#endif // CLICK_QUERY_H
