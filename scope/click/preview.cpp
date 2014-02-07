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

#include "preview.h"
#include<unity-scopes.h>

using namespace unity::scopes;
namespace click {

void Preview::run(PreviewReplyProxy const& reply)
 {
        PreviewWidgetList widgets;
        widgets.emplace_back(PreviewWidget(R"({"id": "header", "type": "header", "title": "title", "subtitle": 
"author", "rating": "rating"})"));
        widgets.emplace_back(PreviewWidget(R"({"id": "img", "type": "image", "art": "screenshot-url"})"));

        PreviewWidget w("img2", "image");
        w.add_attribute("zoomable", Variant(false));
        w.add_component("art", "screenshot-url");
        widgets.emplace_back(w);

        ColumnLayout layout1col(1);
        layout1col.add_column({"header", "title"});

        ColumnLayout layout2col(2);
        layout2col.add_column({"header", "title"});
        layout2col.add_column({"author", "rating"});

        ColumnLayout layout3col(3);
        layout3col.add_column({"header", "title"});
        layout3col.add_column({"author"});
        layout3col.add_column({"rating"});

        reply->register_layout({layout1col, layout2col, layout3col});
        reply->push(widgets);
        reply->push("author", Variant("Foo"));
        reply->push("rating", Variant("4 blah"));

    }

}
