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
#include "download-manager.h"

#if UNITY_SCOPES_API_HEADERS_NOW_UNDER_UNITY
#include <unity/scopes/PreviewReply.h>
#include <unity/scopes/Variant.h>
#include <unity/scopes/VariantBuilder.h>
#else
#include <scopes/VariantBuilder.h>
#endif

#include <QDebug>

#include <boost/optional.hpp>

#include <sstream>

namespace
{

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

void buildDescriptionAndReviews(const scopes::PreviewReplyProxy& /*reply*/,
                                scopes::PreviewWidgetList& widgets,
                                const click::PackageDetails& details)
{
    if (!details.description.empty())
    {
        scopes::PreviewWidget summary("summary", "text");

        summary.add_attribute("text", scopes::Variant(details.description));
        widgets.push_back(summary);
    }

    //TODO: Add Rating and Reviews when that is supported
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
            {"id", scopes::Variant(click::Preview::Actions::INSTALL_CLICK)},
            {"label", scopes::Variant("Install")},
            {"download_url", scopes::Variant(details.download_url)}
        });
        buttons.add_attribute("actions", builder.end());
        widgets.push_back(buttons);
    }

    buildDescriptionAndReviews(reply, widgets, details);

    reply->push(widgets);
}

void buildErrorPreview(scopes::PreviewReplyProxy const& reply,
                       const std::string& error_message)
{
    scopes::PreviewWidgetList widgets;

    scopes::PreviewWidget header("hdr", "header");
    header.add_attribute("title", scopes::Variant("Error"));
    header.add_attribute("subtitle", scopes::Variant(error_message));
    widgets.push_back(header);

    scopes::PreviewWidget buttons("buttons", "actions");
    scopes::VariantBuilder builder;
    builder.add_tuple({
       {"id", scopes::Variant(click::Preview::Actions::CLOSE_PREVIEW)},
       {"label", scopes::Variant("Close")}
    });
    buttons.add_attribute("actions", builder.end());
    widgets.push_back(buttons);

    reply->push(widgets);
}

void buildLoginErrorPreview(scopes::PreviewReplyProxy const& reply)
{
    scopes::PreviewWidgetList widgets;

    scopes::PreviewWidget header("hdr", "header");
    header.add_attribute("title", scopes::Variant("Login Error"));
    widgets.push_back(header);

    scopes::PreviewWidget buttons("buttons", "actions");
    scopes::VariantBuilder builder;
    builder.add_tuple({
       {"id", scopes::Variant(click::Preview::Actions::OPEN_ACCOUNTS)},
       {"label", scopes::Variant("Go to Accounts")}
    });
    buttons.add_attribute("actions", builder.end());
    widgets.push_back(buttons);

    reply->push(widgets);
}

void buildUninstallConfirmationPreview(scopes::PreviewReplyProxy const& reply)
{
    scopes::PreviewWidgetList widgets;

    scopes::PreviewWidget header("hdr", "header");
    header.add_attribute("title", scopes::Variant("Confirmation"));
    header.add_attribute("subtitle",
                         scopes::Variant("Uninstall this app will delete all the related information. Are you sure you want to uninstall?"));
    widgets.push_back(header);

    scopes::PreviewWidget buttons("buttons", "actions");
    scopes::VariantBuilder builder;
    builder.add_tuple({
       {"id", scopes::Variant(click::Preview::Actions::CLOSE_PREVIEW)},
       {"label", scopes::Variant("Not anymore")}
    });
    builder.add_tuple({
       {"id", scopes::Variant(click::Preview::Actions::CONFIRM_UNINSTALL)},
       {"label", scopes::Variant("Yes Uninstall")}
    });
    buttons.add_attribute("actions", builder.end());
    widgets.push_back(buttons);

    reply->push(widgets);
}

void buildInstalledPreview(scopes::PreviewReplyProxy const& reply,
                           const click::PackageDetails& details)
{
    auto widgets = buildAppPreview(details);

    {
        scopes::PreviewWidget buttons("buttons", "actions");
        scopes::VariantBuilder builder;
        builder.add_tuple(
        {
            {"id", scopes::Variant(click::Preview::Actions::OPEN_CLICK)},
            {"label", scopes::Variant("Open")}
        });
        builder.add_tuple(
        {
            {"id", scopes::Variant(click::Preview::Actions::UNINSTALL_CLICK)},
            {"label", scopes::Variant("Uninstall")}
        });
        buttons.add_attribute("actions", builder.end());
        widgets.push_back(buttons);
    }

    buildDescriptionAndReviews(reply, widgets, details);

    reply->push(widgets);
}

void buildInstallingPreview(scopes::PreviewReplyProxy const& reply,
                            const click::PackageDetails& details,
                            const std::string& object_path)
{
    auto widgets = buildAppPreview(details);

    {
        scopes::PreviewWidget progress("download", "progress");
        scopes::VariantMap tuple;
        tuple["dbus-name"] = "com.canonical.DownloadManager";
        tuple["dbus-object"] = object_path;
        progress.add_attribute("source", scopes::Variant(tuple));
        widgets.push_back(progress);

        scopes::PreviewWidget buttons("buttons", "actions");
        scopes::VariantBuilder builder;
        builder.add_tuple(
        {
            {"id", scopes::Variant(click::Preview::Actions::DOWNLOAD_COMPLETED)},
            {"label", scopes::Variant("*** download_completed")}
        });
        builder.add_tuple(
        {
            {"id", scopes::Variant(click::Preview::Actions::DOWNLOAD_FAILED)},
            {"label", scopes::Variant("*** download_failed")}
        });
        buttons.add_attribute("actions", builder.end());
        widgets.push_back(buttons);
    }

    buildDescriptionAndReviews(reply, widgets, details);

    reply->push(widgets);
}

void buildPurchasingPreview(scopes::PreviewReplyProxy const& reply,
                            const click::PackageDetails& details)
{
    auto widgets = buildAppPreview(details);
    // This widget is not in the api yet
    reply->push(widgets);
}

void buildDefaultPreview(scopes::PreviewReplyProxy const& /*reply*/,
                         const click::PackageDetails& /*details*/)
{
    //TBD
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
    qDebug() << "Preview::Preview()";
}

Preview::~Preview()
{
}

void Preview::cancelled()
{
}

void Preview::run(scopes::PreviewReplyProxy const& reply)
{
    qDebug() << "Preview::run()";

    if (result["installed"].get_bool()) {
        setPreview(Type::INSTALLED);
    } else {
        setPreview(Type::UNINSTALLED);
    }

    if (result["name"].get_string().empty()) {
        click::PackageDetails details;
        details.title = result.title();
        details.icon_url = result.art();
        details.description = result["description"].get_string();
        details.main_screenshot_url = result["main_screenshot"].get_string();
        showPreview(reply, details);
    } else {
        // I think this should not be required when we switch the click::Index over
        // to using the Qt bridge. With that, the qt dependency becomes an implementation detail
        // and code using it does not need to worry about threading/event loop topics.
        qt::core::world::enter_with_task([this, reply](qt::core::world::Environment&)
        {
            index->get_details(result["name"].get_string(), [this, reply](const click::PackageDetails& details)
            {
                showPreview(reply, details);
            });
        });
    }
}

void Preview::showPreview(scopes::PreviewReplyProxy const& reply,
                 const click::PackageDetails& details)
{
    switch(type) {
        case Type::UNINSTALLED:
            buildUninstalledPreview(reply, details);
            break;
        case Type::LOGIN:
            buildLoginErrorPreview(reply);
            break;
        case Type::UNINSTALL:
            buildUninstallConfirmationPreview(reply);
            break;
        case Type::INSTALLED:
            buildInstalledPreview(reply, details);
            break;
        case Type::PURCHASE:
            buildPurchasingPreview(reply, details);
            break;
        case Type::DEFAULT:
        case Type::ERROR:
            // Don't showPreview() with errors. always use the error string.
        default:
            qDebug() << "reached default preview type, returning internal error preview";
            buildErrorPreview(reply,
                              std::string("Internal Error, please close and try again."));
            break;
    };
}

void Preview::setPreview(click::Preview::Type type)
{
    this->type = type;
}

InstallPreview::InstallPreview(const std::string &download_url, const QSharedPointer<Index> &index,
                               const unity::scopes::Result &result,
                               const QSharedPointer<click::network::AccessManager> &nam)
    : Preview(result.uri(), index, result), download_url(download_url), 
      downloader(new click::Downloader(nam))
{
    qDebug() << "in InstallPreview constructor, download_url is " << QString::fromStdString(download_url);
}

InstallPreview::~InstallPreview()
{

}

void InstallPreview::run(const unity::scopes::PreviewReplyProxy &reply)
{
    qDebug() << "about to call startDownload in run()";
    downloader->startDownload(download_url, result["name"].get_string(),[this, reply](std::pair<std::string, boost::optional<std::string> > pair) {
            auto obj_path = pair.first;
            auto error = pair.second;
            if (!error) {
                qDebug() << "got object path: " << QString::fromStdString(obj_path);
                index->get_details(result["name"].get_string(), [this, reply, obj_path](const click::PackageDetails& details)
                                   {
                                       buildInstallingPreview(reply, details, obj_path);
                                   });
            } else { 
                buildErrorPreview(reply, *error);
            }
        });
    qDebug() << "after startDownload in run()";
}

} // namespace click
