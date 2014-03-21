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

#include <unity/scopes/PreviewReply.h>
#include <unity/scopes/Variant.h>
#include <unity/scopes/VariantBuilder.h>

#include <QDebug>

#include <functional>
#include <iostream>
#include <sstream>

namespace click {

// Preview base class

Preview::Preview(const unity::scopes::Result& result,
                 const QSharedPointer<click::Index>& index) :
    result(result), index(index)
{
}

Preview::~Preview()
{
}

void Preview::cancelled()
{
}


// TODO: error handling - once get_details provides errors, we can
// return them from populateDetails and check them in the calling code
// to decide whether to show error widgets. see bug LP: #1289541
void Preview::populateDetails(std::function<void(const click::PackageDetails& details)> callback)
{

    std::string app_name = result["name"].get_string();

    if (app_name.empty()) {
        click::PackageDetails details;
        qDebug() << "in populateDetails(), app_name is empty";
        details.title = result.title();
        details.icon_url = result.art();
        details.description = result["description"].get_string();
        details.main_screenshot_url = result["main_screenshot"].get_string();
        callback(details);
    } else {
        qDebug() << "in populateDetails(), app_name is NOT empty, we must hit index:";
        // I think this should not be required when we switch the click::Index over
        // to using the Qt bridge. With that, the qt dependency becomes an implementation detail
        // and code using it does not need to worry about threading/event loop topics.
        qt::core::world::enter_with_task([this, callback](qt::core::world::Environment&)
            {
                index->get_details(result["name"].get_string(), callback);
            });
    }
}

scopes::PreviewWidgetList Preview::headerWidgets(const click::PackageDetails& details)
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
            for (auto const& s: details.more_screenshots_urls)
            {
                arr.push_back(scopes::Variant(s));
            }
        }

        gallery.add_attribute_value("sources", scopes::Variant(arr));
        widgets.push_back(gallery);
    }

    scopes::PreviewWidget header("hdr", "header");
    header.add_attribute_value("title", scopes::Variant(details.title));
    if (!details.description.empty())
    {
        std::stringstream ss(details.description);
        std::string first_line;
        if (std::getline(ss, first_line))
            header.add_attribute_value("subtitle", scopes::Variant(first_line));
    }
    if (!details.icon_url.empty())
        header.add_attribute_value("mascot", scopes::Variant(details.icon_url));
    widgets.push_back(header);

    return widgets;
}

scopes::PreviewWidgetList Preview::descriptionWidgets(const click::PackageDetails& details)
{
    scopes::PreviewWidgetList widgets;
    if (details.description.empty())
    {
        return widgets;
    }

    scopes::PreviewWidget summary("summary", "text");
    summary.add_attribute_value("text", scopes::Variant(details.description));
    widgets.push_back(summary);

    return widgets;
}

scopes::PreviewWidgetList Preview::downloadErrorWidgets()
{
    return errorWidgets(scopes::Variant("Download Error"),
                        scopes::Variant("Download or install failed. Please try again."),
                        scopes::Variant(click::Preview::Actions::CLOSE_PREVIEW), // TODO see bug LP: #1289434
                        scopes::Variant("Close"));
}

scopes::PreviewWidgetList Preview::loginErrorWidgets()
{
    return errorWidgets(scopes::Variant("Login Error"),
                        scopes::Variant("Please log in to your Ubuntu One account."),
                        scopes::Variant(click::Preview::Actions::OPEN_ACCOUNTS),
                        scopes::Variant("Go to Accounts"));
}

scopes::PreviewWidgetList Preview::errorWidgets(const scopes::Variant& title,
                                                const scopes::Variant& subtitle,
                                                const scopes::Variant& action_id,
                                                const scopes::Variant& action_label)
{
    scopes::PreviewWidgetList widgets;

    scopes::PreviewWidget header("hdr", "header");
    header.add_attribute_value("title", title);
    header.add_attribute_value("subtitle", subtitle);
    widgets.push_back(header);

    scopes::PreviewWidget buttons("buttons", "actions");
    scopes::VariantBuilder builder;
    builder.add_tuple({ {"id", action_id}, {"label", action_label} });
    buttons.add_attribute_value("actions", builder.end());
    widgets.push_back(buttons);

    return widgets;
}


// class DownloadErrorPreview

DownloadErrorPreview::DownloadErrorPreview(const unity::scopes::Result &result,
                                           const QSharedPointer<click::Index>& index)
    : Preview(result, index)
{
}

DownloadErrorPreview::~DownloadErrorPreview()
{

}

void DownloadErrorPreview::run(const unity::scopes::PreviewReplyProxy &reply)
{
    // NOTE: no details used by downloadErrorWidgets(), so no need to
    // call populateDetails() here.
    reply->push(downloadErrorWidgets());
}


// class InstallingPreview

InstallingPreview::InstallingPreview(const std::string &download_url,
                                     const unity::scopes::Result &result,
                                     const QSharedPointer<Index> &index,
                                     const QSharedPointer<click::network::AccessManager> &nam)
    : Preview(result, index), download_url(download_url),
      downloader(new click::Downloader(nam))
{
}

InstallingPreview::~InstallingPreview()
{
}

void InstallingPreview::run(const unity::scopes::PreviewReplyProxy &reply)
{
    downloader->startDownload(download_url, result["name"].get_string(),
            [this, reply] (std::pair<std::string, click::InstallError> rc){
              // NOTE: details not needed by fooErrorWidgets, so no need to populateDetails():
              switch (rc.second)
              {
              case InstallError::CredentialsError:
                  qWarning() << "InstallingPreview got error in getting credentials during startDownload";
                  reply->push(loginErrorWidgets());
                  return;
              case InstallError::DownloadInstallError:
                  qWarning() << "Error received from UDM during startDownload:" << rc.first.c_str();
                  reply->push(downloadErrorWidgets());
                  return;
              default:
                  qDebug() << "Successfully created UDM Download.";
                  populateDetails([this, reply, rc](const PackageDetails &details){
                      reply->push(headerWidgets(details));
                      reply->push(progressBarWidget(rc.first));
                      reply->push(descriptionWidgets(details));
                  });
                  break;
              }
            });

}

scopes::PreviewWidgetList InstallingPreview::progressBarWidget(const std::string& object_path)
{
    scopes::PreviewWidgetList widgets;
    scopes::PreviewWidget progress("download", "progress");
    scopes::VariantMap tuple;
    tuple["dbus-name"] = "com.canonical.applications.Downloader";
    tuple["dbus-object"] = object_path;
    progress.add_attribute_value("source", scopes::Variant(tuple));
    widgets.push_back(progress);

    return widgets;
}


// class InstalledPreview

InstalledPreview::InstalledPreview(const unity::scopes::Result& result,
                                   const QSharedPointer<click::Index>& index)
    : Preview(result, index)
{
}

InstalledPreview::~InstalledPreview()
{
}

void InstalledPreview::run(unity::scopes::PreviewReplyProxy const& reply)
{
    populateDetails([this, reply](const PackageDetails &details){
        reply->push(headerWidgets(details));
        reply->push(installedActionButtonWidgets());
        reply->push(descriptionWidgets(details));
    });
}

scopes::PreviewWidgetList InstalledPreview::installedActionButtonWidgets()
{
    scopes::PreviewWidgetList widgets;

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
    buttons.add_attribute_value("actions", builder.end());
    widgets.push_back(buttons);

    return widgets;
}


// class PurchasingPreview

PurchasingPreview::PurchasingPreview(const unity::scopes::Result& result,
                                     const QSharedPointer<click::Index>& index)
    : Preview(result, index)
{
}

PurchasingPreview::~PurchasingPreview()
{
}

void PurchasingPreview::run(unity::scopes::PreviewReplyProxy const& reply)
{
    populateDetails([this, reply](const PackageDetails &details){
        reply->push(purchasingWidgets(details));
    });
}


scopes::PreviewWidgetList PurchasingPreview::purchasingWidgets(const PackageDetails &/*details*/)
{
    scopes::PreviewWidgetList widgets;
    return widgets;
}


// class UninstallConfirmationPreview

UninstallConfirmationPreview::UninstallConfirmationPreview(const unity::scopes::Result& result,
                                                           const QSharedPointer<click::Index>& index)
    : Preview(result, index)
{
}

UninstallConfirmationPreview::~UninstallConfirmationPreview()
{
}

void UninstallConfirmationPreview::run(unity::scopes::PreviewReplyProxy const& reply)
{
    // NOTE: no need to populateDetails() here.
    scopes::PreviewWidgetList widgets;

    scopes::PreviewWidget header("hdr", "header");
    header.add_attribute_value("title", scopes::Variant("Confirmation"));
    header.add_attribute_value("subtitle",
                         scopes::Variant("Uninstalling this app will delete all the related information. Are you sure you want to uninstall?")); // TODO: wording needs review. see bug LP: #1234211
    widgets.push_back(header);

    scopes::PreviewWidget buttons("buttons", "actions");
    scopes::VariantBuilder builder;
    builder.add_tuple({
       {"id", scopes::Variant(click::Preview::Actions::CLOSE_PREVIEW)}, // TODO: see bug LP: #1289434
       {"label", scopes::Variant("Not anymore")}
    });
    builder.add_tuple({
       {"id", scopes::Variant(click::Preview::Actions::CONFIRM_UNINSTALL)},
       {"label", scopes::Variant("Yes Uninstall")}
    });
    buttons.add_attribute_value("actions", builder.end());
    widgets.push_back(buttons);

    reply->push(widgets);
}

// class UninstalledPreview

UninstalledPreview::UninstalledPreview(const unity::scopes::Result& result,
                                       const QSharedPointer<click::Index>& index)
    :Preview(result, index)
{
}

UninstalledPreview::~UninstalledPreview()
{
}

void UninstalledPreview::run(unity::scopes::PreviewReplyProxy const& reply)
{
qDebug() << "in UninstalledPreview::run, about to populate details";

    populateDetails([this, reply](const PackageDetails &details){
        reply->push(headerWidgets(details));
        reply->push(uninstalledActionButtonWidgets(details));
        reply->push(descriptionWidgets(details));
    });
}

scopes::PreviewWidgetList UninstalledPreview::uninstalledActionButtonWidgets(const PackageDetails &details)
{
    scopes::PreviewWidgetList widgets;
    scopes::PreviewWidget buttons("buttons", "actions");
    scopes::VariantBuilder builder;
    builder.add_tuple(
        {
            {"id", scopes::Variant(click::Preview::Actions::INSTALL_CLICK)},
            {"label", scopes::Variant("Install")},
            {"download_url", scopes::Variant(details.download_url)}
        });
    buttons.add_attribute_value("actions", builder.end());
    widgets.push_back(buttons);

    return widgets;
}

// class UninstallingPreview : public UninstalledPreview

// TODO: this class should be removed once uninstall() is handled elsewhere.
UninstallingPreview::UninstallingPreview(const unity::scopes::Result& result,
                                         const QSharedPointer<click::Index>& index)
    : UninstalledPreview(result, index)
{
}
UninstallingPreview::~UninstallingPreview()
{
}

void UninstallingPreview::run(unity::scopes::PreviewReplyProxy const& reply)
{
    qDebug() << "in UninstallingPreview::run, calling uninstall";
    uninstall();
    qDebug() << "in UninstallingPreview::run, calling UninstalledPreview::run()";
    UninstalledPreview::run(reply);
}

void UninstallingPreview::uninstall()
{
    click::Package package;
    package.title = result.title();
    package.name = result["name"].get_string();
    package.version = result["version"].get_string();
    qt::core::world::enter_with_task([this, package] (qt::core::world::Environment& /*env*/)
    {
        click::PackageManager manager;
        manager.uninstall(package, [&](int code, std::string stderr_content) {
                if (code != 0) {
                    qDebug() << "Error removing package:" << stderr_content.c_str();
                } else {
                    qDebug() << "successfully removed package";

                }
            } );
    });
}


} // namespace click
