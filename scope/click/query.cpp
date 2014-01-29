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

#include "query.h"

#if UNITY_SCOPES_API_HEADERS_NOW_UNDER_UNITY
#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/Query.h>
#include <unity/scopes/SearchReply.h>
#else
#include <scopes/Annotation.h>
#include <scopes/CategoryRenderer.h>
#include <scopes/CategorisedResult.h>
#include <scopes/Query.h>
#include <scopes/SearchReply.h>
#endif

click::Query::Query(std::string const& query)
    : query(query)
{
}

click::Query::~Query()
{
}

void click::Query::cancelled()
{
}

void click::Query::run(scopes::SearchReplyProxy const& reply)
{
    scopes::CategoryRenderer rdr;
    auto cat = reply->register_category("cat1", "Category 1", "", rdr);
    scopes::CategorisedResult res(cat);
    res.set_uri("uri");
    res.set_title("scope-A: result 1 for query \"" + query + "\"");
    res.set_art("icon");
    res.set_dnd_uri("dnd_uri");
    reply->push(res);

    scopes::Query q("scope-A", query, "");
    scopes::Annotation annotation(scopes::Annotation::Type::Link);
    annotation.add_link("More...", q);
    reply->push(annotation);
}
