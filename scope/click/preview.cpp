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
#include <QDebug>

#define ACTION_INSTALL_CLICK "install_click"
#define ACTION_BUY_CLICK "buy_click"
#define ACTION_DOWNLOAD_COMPLETED "finished"
#define ACTION_DOWNLOAD_FAILED "failed"
#define ACTION_PURCHASE_SUCCEEDED "purchase_succeeded"
#define ACTION_PURCHASE_FAILED "purchase_failed"
#define ACTION_OPEN_CLICK "open_click"
#define ACTION_PIN_TO_LAUNCHER "pin_to_launcher"
#define ACTION_UNINSTALL_CLICK "uninstall_click"
#define ACTION_CONFIRM_UNINSTALL "confirm_uninstall"
#define ACTION_CLOSE_PREVIEW "close_preview"
#define ACTION_OPEN_ACCOUNTS "open_accounts"

using namespace unity::scopes;
namespace click {

Preview::Preview(std::string const& uri, click::Index* index, const unity::scopes::Result& result) :
    uri_(uri),
    message_(""),
    index_(index),
    result_(result)
{
    setPreview(PREVIEWS::UNINSTALLED);
    qDebug() << "\n" << QString::fromStdString(result_["name"].get_string()) << "\n";
    index_->get_details(result_["name"].get_string(), [&](click::PackageDetails details) {
        qDebug() << "\n\n" << QString::fromStdString(details.name) << "\n\n";
    });
}

void Preview::run(PreviewReplyProxy const& reply)
 {
    message_ = "";
    switch(type_) {
        case PREVIEWS::ERROR:
        case PREVIEWS::LOGIN:
        case PREVIEWS::UNINSTALL:
        case PREVIEWS::UNINSTALLED:
            buildUninstalledPreview(reply);
            break;
        case PREVIEWS::INSTALLED:
        case PREVIEWS::INSTALLING:
        case PREVIEWS::PURCHASE:
        case PREVIEWS::DEFAULT:
        default:
            buildErrorPreview(reply);
            break;
    }
}

void Preview::setPreview(PREVIEWS type)
{
    type_ = type;
}

PreviewWidgetList Preview::buildAppPreview(PreviewReplyProxy const& /*reply*/)
{
    PreviewWidgetList widgets;

    PreviewWidget gallery("screenshots", "gallery");
    VariantArray arr;
    arr.push_back(Variant("http://ubuntuone.com/3MVMKyhTe528kfcP2wmvjj"));
    arr.push_back(Variant("http://ubuntuone.com/4QZWz6zdeNX4OJAk0zVuOi"));
    arr.push_back(Variant("http://ubuntuone.com/1zuZfnxZbpCVSlmMf2ctui"));
    gallery.add_attribute("sources", Variant(arr));
    widgets.emplace_back(gallery);

    PreviewWidget header("hdr", "header");
    header.add_attribute("title", Variant("Title"));
    header.add_attribute("subtitle", Variant("Subtitle"));
    header.add_attribute("mascot", Variant("https://raw2.github.com/ninja-ide/ninja-ide/master/ninja_ide/img/icon.png"));
    widgets.emplace_back(header);

    return widgets;
}

void Preview::buildErrorPreview(PreviewReplyProxy const& reply)
{
    PreviewWidgetList widgets;

    PreviewWidget header("hdr", "header");
    header.add_attribute("title", Variant("Error"));
    widgets.emplace_back(header);

    PreviewWidget buttons("buttons", "actions");
    VariantBuilder builder;
    builder.add_tuple({
       {"id", Variant(ACTION_CLOSE_PREVIEW)},
       {"label", Variant("Close")}
    });
    buttons.add_attribute("actions", builder.end());
    widgets.emplace_back(buttons);

    reply->push(widgets);
}

void Preview::buildUninstalledPreview(PreviewReplyProxy const& reply)
{
    PreviewWidgetList widgets = buildAppPreview(reply);

    PreviewWidget buttons("buttons", "actions");
    VariantBuilder builder;
    builder.add_tuple({
       {"id", Variant(ACTION_INSTALL_CLICK)},
       {"label", Variant("Install")}
    });
    buttons.add_attribute("actions", builder.end());
    widgets.emplace_back(buttons);

    PreviewWidget summary("summary", "text");
    summary.add_attribute("text", Variant("NINJA-IDE (from the recursive acronym: \"Ninja-IDE Is Not Just Another IDE\"), is a cross-platform integrated development environment (IDE). NINJA-IDE runs on Linux/X11, Mac OS X and Windows desktop operating systems, and allows developers to create applications for several purposes using all the tools and utilities of NINJA-IDE, making the task of writing software easier and more enjoyable."));
    widgets.emplace_back(summary);

    reply->push(widgets);
}

}
