/*
 * Copyright (C) 2014-2016 Canonical Ltd.
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

#include <click/application.h>
#include <click/interface.h>
#include "preview.h"
#include <click/qtbridge.h>
#include <click/download-manager.h>
#include <click/launcher.h>
#include <click/dbus_constants.h>
#include <click/departments-db.h>
#include <click/utils.h>
#include <click/pay.h>

#include <boost/algorithm/string/replace.hpp>

#include <unity/UnityExceptions.h>
#include <unity/scopes/CannedQuery.h>
#include <unity/scopes/PreviewReply.h>
#include <unity/scopes/Variant.h>
#include <unity/scopes/VariantBuilder.h>
#include <unity/scopes/ColumnLayout.h>

#include <QCoreApplication>
#include <QDebug>

#include <functional>
#include <iostream>
#include <regex>
#include <sstream>

#include <click/click-i18n.h>

namespace click {

void WidgetsInColumns::registerLayouts(unity::scopes::PreviewReplyProxy const& reply)
{
    unity::scopes::ColumnLayout layout1col(1);
    layout1col.add_column(singleColumn.column1);

    unity::scopes::ColumnLayout layout2col(2);
    layout2col.add_column(twoColumns.column1);
    layout2col.add_column(twoColumns.column2);

    try
    {
        reply->register_layout({layout1col, layout2col});
    }
    catch (unity::LogicException const& e)
    {
        qWarning() << "Failed to register layout:" << QString::fromStdString(e.what());
    }
}

void WidgetsInColumns::appendToColumn(std::vector<std::string>& column, unity::scopes::PreviewWidgetList const& widgets)
{
    for (auto const& widget: widgets)
    {
        column.push_back(widget.id());
    }
}

DepartmentUpdater::DepartmentUpdater(const std::shared_ptr<click::DepartmentsDb>& depts)
    : depts(depts)
{
}

// Preview base class

Preview::Preview(const unity::scopes::Result& result,
                 const unity::scopes::ActionMetadata& metadata,
                 std::shared_future<void> const& qt_ready)
    : PreviewQueryBase(result, metadata),
      result(result),
      metadata(metadata),
      qt_ready_(qt_ready)
{
}

Preview::~Preview()
{
}

void Preview::choose_strategy(const QSharedPointer<web::Client> &client,
                              const QSharedPointer<pay::Package>& ppackage,
                              const QSharedPointer<Ubuntu::DownloadManager::Manager>& manager,
                              std::shared_ptr<click::DepartmentsDb> depts)
{
    strategy.reset(build_strategy(result, metadata, client, ppackage, manager, depts));
}


PreviewStrategy* Preview::build_strategy(const unity::scopes::Result &result,
                                         const unity::scopes::ActionMetadata &metadata,
                                         const QSharedPointer<web::Client> &client,
                                         const QSharedPointer<pay::Package>& ppackage,
                                         const QSharedPointer<Ubuntu::DownloadManager::Manager>& manager,
                                         std::shared_ptr<click::DepartmentsDb> depts)
{
    if (metadata.scope_data().which() != scopes::Variant::Type::Null) {
        auto metadict = metadata.scope_data().get_dict();

        if (metadict.count(click::Preview::Actions::SHOW_INSTALLED) != 0) {
            qDebug() << "in Scope::preview(),"
                     << " and close_preview="
                     << metadict.count(click::Preview::Actions::SHOW_INSTALLED);

            return new InstalledPreview(result, metadata, client, ppackage, depts);
        } else if (metadict.count(click::Preview::Actions::UNINSTALL_CLICK) != 0) {
            return new UninstallConfirmationPreview(result);
        } else if (metadict.count(click::Preview::Actions::CONFIRM_UNINSTALL) != 0) {
            return new UninstallingPreview(result, metadata, client, manager, ppackage);
        } else if (metadict.count(click::Preview::Actions::SHOW_UNINSTALLED) != 0) {
            return new UninstalledPreview(result, metadata, client, depts, manager, ppackage);
        } else {
            qWarning() << "preview() called with unexpected metadata. returning uninstalled preview";
            return new UninstalledPreview(result, metadata, client, depts, manager, ppackage);
        }
    } else {
        // metadata.scope_data() is Null, so we return an appropriate "default" preview:
        if (result.uri().find("scope://") == 0)
        {
            return new InstalledScopePreview(result);
        }
        if (result["installed"].get_bool() == true) {
            return new InstalledPreview(result, metadata, client, ppackage, depts);
        } else {
            return new UninstalledPreview(result, metadata, client, depts, manager, ppackage);
        }
    }

}

void Preview::cancelled()
{
    strategy->cancelled();
}

void Preview::run(const unity::scopes::PreviewReplyProxy &reply)
{
    if (qt_ready_.valid())
        qt_ready_.wait();

    strategy->run(reply);
}

PreviewStrategy::PreviewStrategy(const unity::scopes::Result& result)
    : result(result),
      oa_client("ubuntuone", "ubuntuone", "ubuntuone",
                scopes::OnlineAccountClient::MainLoopSelect::CreateInternalMainLoop)
{
}

PreviewStrategy::PreviewStrategy(const unity::scopes::Result& result,
                 const QSharedPointer<click::web::Client>& client) :
    result(result),
    client(client),
    index(new click::Index(client)),
    reviews(new click::Reviews(client)),
    oa_client("ubuntuone", "ubuntuone", "ubuntuone",
              scopes::OnlineAccountClient::MainLoopSelect::CreateInternalMainLoop)
{
}

PreviewStrategy::PreviewStrategy(const unity::scopes::Result& result,
                                 const QSharedPointer<click::web::Client>& client,
                                 const QSharedPointer<pay::Package>& ppackage)
    : result(result),
      client(client),
      index(new click::Index(client)),
      reviews(new click::Reviews(client)),
      oa_client("ubuntuone", "ubuntuone", "ubuntuone",
                scopes::OnlineAccountClient::MainLoopSelect::CreateInternalMainLoop),
      pay_package(ppackage)
{
}

void PreviewStrategy::pushPackagePreviewWidgets(const unity::scopes::PreviewReplyProxy &reply,
                                                const PackageDetails &details,
                                                const scopes::PreviewWidgetList& button_area_widgets)
{
    reply->push(headerWidgets(details));
    reply->push(button_area_widgets);
}

PreviewStrategy::~PreviewStrategy()
{
}

void PreviewStrategy::cancelled()
{
    index_operation.cancel();
    reviews_operation.cancel();
    submit_operation.cancel();
    purchase_operation.cancel();
}

void PreviewStrategy::run_under_qt(const std::function<void ()> &task)
{
    auto _app = QCoreApplication::instance();
    if (_app != nullptr) {
        qt::core::world::enter_with_task([task]() {
                task();
            });
    } else {
        task();
    }
}

std::string get_string_maybe_null(scopes::Variant variant)
{
    if (variant.is_null()) {
        return "";
    } else {
        return variant.get_string();
    }
}

// TODO: error handling - once get_details provides errors, we can
// return them from populateDetails and check them in the calling code
// to decide whether to show error widgets. see bug LP: #1289541
void PreviewStrategy::populateDetails(std::function<void(const click::PackageDetails& details)> details_callback)
{

    //std::string app_name = get_string_maybe_null(result["name"]);

    click::PackageDetails details;
    qDebug() << "in populateDetails(), app_name is empty";
    details.package.title = result.title();
    details.package.icon_url = result.art();
    details_callback(details);
}

scopes::PreviewWidgetList PreviewStrategy::headerWidgets(const click::PackageDetails& details)
{
    scopes::PreviewWidgetList widgets;

    scopes::PreviewWidget header("hdr", "header");
    header.add_attribute_value("title", scopes::Variant(result.title()));
    if (!details.publisher.empty())
    {
        header.add_attribute_value("subtitle", scopes::Variant(details.publisher));
    }
    if (!details.package.icon_url.empty())
    {
        header.add_attribute_value("mascot", scopes::Variant(details.package.icon_url));
    }

    widgets.push_back(header);

    qDebug() << "Pushed widgets for package:" << QString::fromStdString(details.package.name);
    return widgets;
}

void PreviewStrategy::invalidateScope(const std::string& scope_id)
{
    run_under_qt([scope_id]() {
            PackageManager::invalidate_results(scope_id);
        });
}

// class InstalledPreview

InstalledPreview::InstalledPreview(const unity::scopes::Result& result,
                                   const unity::scopes::ActionMetadata& metadata,
                                   const QSharedPointer<click::web::Client>& client,
                                   const QSharedPointer<pay::Package>& ppackage,
                                   const std::shared_ptr<click::DepartmentsDb>& depts)
    : PreviewStrategy(result, client, ppackage),
      DepartmentUpdater(depts),
      metadata(metadata)
{
}

InstalledPreview::~InstalledPreview()
{
}


void InstalledPreview::run(unity::scopes::PreviewReplyProxy const& reply)
{
    // Get the click manifest.
    Manifest manifest;
    std::promise<Manifest> manifest_promise;
    std::future<Manifest> manifest_future = manifest_promise.get_future();
    std::string app_name = result["name"].get_string();
    if (!app_name.empty()) {
        qt::core::world::enter_with_task([&]() {
            click::Interface().get_manifest_for_app(app_name,
                [&](Manifest found_manifest, InterfaceError error) {
                    qDebug() << "Got manifest for:" << app_name.c_str();

                    if (error != click::InterfaceError::NoError) {
                        qDebug() << "There was an error getting the manifest for:" << app_name.c_str();
                    }
                    manifest_promise.set_value(found_manifest);
            });
        });
        manifest = manifest_future.get();
    }

    populateDetails([this, reply, manifest](const PackageDetails &details){
        pushPackagePreviewWidgets(reply, details, createButtons(manifest));
    });
}

scopes::PreviewWidgetList InstalledPreview::createButtons(const Manifest& manifest)
{
    scopes::PreviewWidgetList widgets;
    scopes::PreviewWidget buttons("buttons", "actions");
    scopes::VariantBuilder builder;

    std::string open_label = _("Open");

    auto uri = getApplicationUri(manifest);

    if (!manifest.has_any_apps() && manifest.has_any_scopes()) {
        open_label = _("Search");
    }

    if (!uri.empty())
    {
        builder.add_tuple(
        {
            {"id", scopes::Variant(click::Preview::Actions::OPEN_CLICK)},
            {"label", scopes::Variant(open_label)},
            {"uri", scopes::Variant(uri)}
                    });
        qDebug() << "Adding button" << QString::fromStdString(open_label) << "-"
                 << QString::fromStdString(uri);
    }
    if (manifest.removable)
    {
        builder.add_tuple({
                              {"id", scopes::Variant(click::Preview::Actions::UNINSTALL_CLICK)},
                              {"label", scopes::Variant(_("Uninstall"))}
                          });
    }
    if (!uri.empty() || manifest.removable) {
        buttons.add_attribute_value("actions", builder.end());
        widgets.push_back(buttons);
    }
    return widgets;
}

std::string InstalledPreview::getApplicationUri(const Manifest& manifest)
{
    static std::regex appurl_match{"^(application|appid)://[a-zA-Z\\._/-]+$"};

    if (!std::regex_match(result.uri(), appurl_match)) {
        if (manifest.has_any_apps()) {
            std::string uri = "appid://" + manifest.name + "/" +
                manifest.first_app_name + "/current-user-version";
            return uri;
        } else if (manifest.has_any_scopes()) {
            unity::scopes::CannedQuery cq(manifest.first_scope_id);
            auto scope_uri = cq.to_uri();
            qDebug() << "Found uri for scope"
                     << QString::fromStdString(manifest.first_scope_id)
                     << "-" << QString::fromStdString(scope_uri);
            return scope_uri;
        } else {
            qWarning() << "Unable to find app or scope URI for:"
                       << QString::fromStdString(manifest.name);
            return "";
        }
    }
    return result.uri();
}

// class InstalledScopePreview
// this is a temporary fallback preview to get into the Store scope, the proper
// requires 'store' category to be treated special (like 'local') in unity8 shell.

InstalledScopePreview::InstalledScopePreview(const unity::scopes::Result& result)
    : PreviewStrategy(result)
{
}

void InstalledScopePreview::run(unity::scopes::PreviewReplyProxy const& reply)
{
    scopes::PreviewWidget actions("actions", "actions");
    {
        scopes::VariantBuilder builder;
        builder.add_tuple({
                {"id", scopes::Variant("search")},
                {"uri", scopes::Variant(result.uri())},
                {"label", scopes::Variant(_("Search"))}
            });
        actions.add_attribute_value("actions", builder.end());
    }

    reply->push({actions});
}

// class UninstallConfirmationPreview

UninstallConfirmationPreview::UninstallConfirmationPreview(const unity::scopes::Result& result)
    : PreviewStrategy(result)
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
    header.add_attribute_value("title", scopes::Variant(_("Confirmation")));
    std::string title = result["title"].get_string();
    // TRANSLATORS: Do NOT translate ${title} here.
    std::string message = _("Uninstall ${title}?");
    boost::replace_first(message, "${title}", title);
    header.add_attribute_value("subtitle", scopes::Variant(message));
    widgets.push_back(header);

    scopes::PreviewWidget buttons("buttons", "actions");
    scopes::VariantBuilder builder;
    builder.add_tuple({
       {"id", scopes::Variant(click::Preview::Actions::SHOW_INSTALLED)},
       {"label", scopes::Variant(_("Cancel"))}
    });
    builder.add_tuple({
       {"id", scopes::Variant(click::Preview::Actions::CONFIRM_UNINSTALL)},
       {"label", scopes::Variant(_("Confirm"))}
    });
    buttons.add_attribute_value("actions", builder.end());
    widgets.push_back(buttons);

    reply->push(widgets);
}

// class UninstalledPreview

UninstalledPreview::UninstalledPreview(const unity::scopes::Result& result,
                                       const unity::scopes::ActionMetadata& metadata,
                                       const QSharedPointer<click::web::Client>& client,
                                       const std::shared_ptr<click::DepartmentsDb>& depts,
                                       const QSharedPointer<Ubuntu::DownloadManager::Manager>& manager,
                                       const QSharedPointer<pay::Package>& ppackage)
    : PreviewStrategy(result, client, ppackage),
      DepartmentUpdater(depts),
      metadata(metadata),
      dm(new DownloadManager(client, manager))
{
    qDebug() << "Creating new UninstalledPreview for result" << QString::fromStdString(result["name"].get_string());
}

UninstalledPreview::~UninstalledPreview()
{
}

void UninstalledPreview::run(unity::scopes::PreviewReplyProxy const& reply)
{
    qDebug() << "in UninstalledPreview::run, about to populate details";
    populateDetails([this, reply](const PackageDetails &details){
            found_details = details;
        });
}

scopes::PreviewWidgetList UninstalledPreview::uninstalledActionButtonWidgets(const PackageDetails &/*details*/)
{
    scopes::PreviewWidgetList widgets;

    return widgets;
}

// class UninstallingPreview : public UninstalledPreview

// TODO: this class should be removed once uninstall() is handled elsewhere.
UninstallingPreview::UninstallingPreview(const unity::scopes::Result& result,
                                         const unity::scopes::ActionMetadata& metadata,
                                         const QSharedPointer<click::web::Client>& client,
                                         const QSharedPointer<Ubuntu::DownloadManager::Manager>& manager,
                                         const QSharedPointer<pay::Package>& ppackage)
    : UninstalledPreview(result, metadata, client, nullptr, manager, ppackage)
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
    qt::core::world::enter_with_task([this, package] ()
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
