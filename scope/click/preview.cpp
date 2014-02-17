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
#include "qtbridge.h"

#if UNITY_SCOPES_API_HEADERS_NOW_UNDER_UNITY
#include <unity/scopes/PreviewReply.h>
#include <unity/scopes/Variant.h>
#include <unity/scopes/VariantBuilder.h>
#else
#include <scopes/VariantBuilder.h>
#endif

#include <QDebug>

#include <sstream>

namespace
{
namespace actions
{
static const std::string INSTALL_CLICK = "install_click";
static const std::string BUY_CLICK = "buy_click";
static const std::string DOWNLOAD_COMPLETED = "finished";
static const std::string DOWNLOAD_FAILED = "failed";
static const std::string PURCHASE_SUCCEEDED = "purchase_succeeded";
static const std::string PURCHASE_FAILED = "purchase_failed";
static const std::string OPEN_CLICK = "open_click";
static const std::string PIN_TO_LAUNCHER = "pin_to_launcher";
static const std::string UNINSTALL_CLICK = "uninstall_click";
static const std::string CONFIRM_UNINSTALL = "confirm_uninstall";
static const std::string CLOSE_PREVIEW = "close_preview";
static const std::string OPEN_ACCOUNTS = "open_accounts";
}

scopes::PreviewWidgetList buildAppPreview(const click::PackageDetails& details)
{
    scopes::PreviewWidgetList widgets;

    bool has_screenshots = !details.main_screenshot_url.empty() || !details.more_screenshots_urls.empty();

    if (has_screenshots)
    {
        scopes::PreviewWidget gallery("screenshots", "gallery");
        scopes::VariantArray arr;

        if (!details.main_screenshot_url.empty())
            arr.push_back(scopes::Variant(details.main_screenshot_url));
        if (!details.more_screenshots_urls.empty())
        {
            // TODO: Tokenize list of screenshot urls here.
        }

        gallery.add_attribute("sources", scopes::Variant(arr));
        widgets.push_back(gallery);
    }

    scopes::PreviewWidget header("hdr", "header");
    header.add_attribute("title", scopes::Variant(details.title));
    if (!details.description.empty())
    {
        std::stringstream ss(details.description);
        std::string first_line;
        if (std::getline(ss, first_line))
            header.add_attribute("subtitle", scopes::Variant(first_line));
    }
    if (!details.icon_url.empty())
        header.add_attribute("mascot", scopes::Variant(details.icon_url));
    widgets.push_back(header);

    return widgets;
}

void buildUninstalledPreview(const scopes::PreviewReplyProxy& reply,
                             const click::PackageDetails& details)
{
    auto widgets = buildAppPreview(details);

    {
        scopes::PreviewWidget buttons("buttons", "actions");
        scopes::VariantBuilder builder;
        builder.add_tuple(
        {
            {"id", scopes::Variant(actions::INSTALL_CLICK)},
            {"label", scopes::Variant("Install")}
        });
        buttons.add_attribute("actions", builder.end());
        widgets.push_back(buttons);
    }

    if (!details.description.empty())
    {
        scopes::PreviewWidget summary("summary", "text");

        summary.add_attribute("text", scopes::Variant(details.description));
        widgets.push_back(summary);
    }

    reply->push(widgets);
}

void buildErrorPreview(scopes::PreviewReplyProxy const& reply)
{
    scopes::PreviewWidgetList widgets;

    scopes::PreviewWidget header("hdr", "header");
    header.add_attribute("title", scopes::Variant("Error"));
    widgets.push_back(header);

    scopes::PreviewWidget buttons("buttons", "actions");
    scopes::VariantBuilder builder;
    builder.add_tuple({
       {"id", scopes::Variant(actions::CLOSE_PREVIEW)},
       {"label", scopes::Variant("Close")}
    });
    buttons.add_attribute("actions", builder.end());
    widgets.push_back(buttons);

    reply->push(widgets);
}
}

namespace click {

Preview::Preview(std::string const& uri,
                 const QSharedPointer<click::Index>& index,
                 const unity::scopes::Result& result) :
    uri(uri),
    index(index),
    result(result),
    type(Type::UNINSTALLED)
{
}

Preview::~Preview()
{
}

void Preview::cancelled()
{
}

void Preview::run(scopes::PreviewReplyProxy const& reply)
{
    setPreview(Type::UNINSTALLED);

    // I think this should not be required when we switch the click::Index over
    // to using the Qt bridge. With that, the qt dependency becomes an implementation detail
    // and code using it does not need to worry about threading/event loop topics.
    qt::core::world::enter_with_task([this, reply](qt::core::world::Environment&)
    {
        index->get_details(result["name"].get_string(), [this, reply](const click::PackageDetails& details)
        {
            switch(type) {
                case Type::UNINSTALLED:
                    buildUninstalledPreview(reply, details);
                    break;
                case Type::ERROR:
                case Type::LOGIN:
                case Type::UNINSTALL:
                case Type::INSTALLED:
                case Type::INSTALLING:
                case Type::PURCHASE:
                case Type::DEFAULT:
                default:
                    buildErrorPreview(reply);
                    break;
            };
        });
    });
}

void Preview::setPreview(click::Preview::Type type)
{
    this->type = type;
}
}
