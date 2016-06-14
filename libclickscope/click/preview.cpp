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
#include <sstream>

#include <click/click-i18n.h>

namespace click {

void CachedPreviewWidgets::push(unity::scopes::PreviewWidget const &widget)
{
    widgets.push_back(widget);
    widgets_lookup.insert(widget.id());
}

void CachedPreviewWidgets::push(unity::scopes::PreviewWidgetList const &widgetList)
{
    for (auto const& widget: widgetList)
    {
        push(widget);
    }
}

void CachedPreviewWidgets::flush(unity::scopes::PreviewReplyProxy const& reply)
{
    // A safeguard: if a new widget gets added but missing in the layout, we will get a warning
    // in the logs and layouts will not be registered (single column with all widgets will be used).
    if (widgets.size() != layout.singleColumn.column1.size() ||
            widgets.size() != layout.twoColumns.column1.size() + layout.twoColumns.column2.size())
    {
        qWarning() << "Number of column layouts doesn't match the number of widgets";
    }
    else
    {
        layout.registerLayouts(reply);
    }
    reply->push(widgets);
    widgets.clear();
    widgets_lookup.clear();
}

bool CachedPreviewWidgets::has(std::string const& widget) const
{
    return widgets_lookup.find(widget) != widgets_lookup.end();
}

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

void DepartmentUpdater::store_department(const PackageDetails& details)
{
    //
    // store package -> department mapping in sqlite db
    if (depts)
    {
        if (!details.department.empty())
        {
            try
            {
                depts->store_package_mapping(details.package.name, details.department);
                qDebug() << "Storing mapping for" << QString::fromStdString(details.package.name) << ":" << QString::fromStdString(details.department);
            }
            catch (const std::exception& e)
            {
                qWarning() << "Failed to store package mapping for package '"
                    << QString::fromStdString(details.package.name)
                    << "', department '" << QString::fromStdString(details.department)
                    << "':" << QString::fromStdString(e.what());
            }
        }
        else
        {
            qWarning() << "Department is empty for package" << QString::fromStdString(details.package.name);
        }
    }
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

PreviewStrategy* Preview::build_installing(const std::string& download_url,
                                           const std::string& download_sha512,
                                           const unity::scopes::Result& result,
                                           const QSharedPointer<click::web::Client>& client,
                                           const QSharedPointer<Ubuntu::DownloadManager::Manager>& manager,
                                           std::shared_ptr<click::DepartmentsDb> depts)
{
    return new InstallingPreview(download_url, download_sha512, result, client, manager, depts);
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

        if (metadict.count(click::Preview::Actions::DOWNLOAD_FAILED) != 0) {
            return new DownloadErrorPreview(result);
        } else if (metadict.count(click::Preview::Actions::DOWNLOAD_COMPLETED) != 0  ||
                   metadict.count(click::Preview::Actions::SHOW_INSTALLED) != 0) {
            qDebug() << "in Scope::preview(), metadata has download_completed="
                     << metadict.count(click::Preview::Actions::DOWNLOAD_COMPLETED)
                     << " and close_preview="
                     << metadict.count(click::Preview::Actions::SHOW_INSTALLED);

            return new InstalledPreview(result, metadata, client, ppackage, depts);
        } else if (metadict.count("action_id") != 0  && metadict.count("download_url") != 0) {
            std::string action_id = metadict["action_id"].get_string();
            std::string download_url = metadict["download_url"].get_string();
            std::string download_sha512 = metadict["download_sha512"].get_string();
            if (action_id == click::Preview::Actions::INSTALL_CLICK) {
                return build_installing(download_url, download_sha512, result, client, manager, depts);
            } else {
                qWarning() << "unexpected action id " << QString::fromStdString(action_id)
                           << " given with download_url" << QString::fromStdString(download_url);
                return new UninstalledPreview(result, metadata, client, depts, manager, ppackage);
            }
        } else if (metadict.count(click::Preview::Actions::CANCEL_PURCHASE_UNINSTALLED) != 0) {
            return new CancelPurchasePreview(result, false);
        } else if (metadict.count(click::Preview::Actions::CANCEL_PURCHASE_INSTALLED) != 0) {
            return new CancelPurchasePreview(result, true);
        } else if (metadict.count(click::Preview::Actions::UNINSTALL_CLICK) != 0) {
            return new UninstallConfirmationPreview(result);
        } else if (metadict.count(click::Preview::Actions::CONFIRM_UNINSTALL) != 0) {
            return new UninstallingPreview(result, metadata, client, manager, ppackage);
        } else if (metadict.count(click::Preview::Actions::CONFIRM_CANCEL_PURCHASE_UNINSTALLED) != 0) {
            return new CancellingPurchasePreview(result, metadata, client, ppackage, manager, false);
        } else if (metadict.count(click::Preview::Actions::CONFIRM_CANCEL_PURCHASE_INSTALLED) != 0) {
            return new CancellingPurchasePreview(result, metadata, client, ppackage, manager, true);
        } else if (metadict.count(click::Preview::Actions::RATED) != 0) {
            return new InstalledPreview(result, metadata, client, ppackage, depts);
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
    reply->push(screenshotsWidgets(details));
    reply->push(descriptionWidgets(details));
}

void PreviewStrategy::pushPackagePreviewWidgets(CachedPreviewWidgets &cache,
                                const PackageDetails& details,
                                const scopes::PreviewWidgetList& button_area_widgets)
{
    cache.push(headerWidgets(details));
    cache.layout.singleColumn.column1.push_back("hdr");
    cache.layout.twoColumns.column1.push_back("hdr");

    cache.push(button_area_widgets);
    cache.layout.appendToColumn(cache.layout.singleColumn.column1, button_area_widgets);
    cache.layout.appendToColumn(cache.layout.twoColumns.column1, button_area_widgets);

    auto const screenshots = screenshotsWidgets(details);
    cache.push(screenshots);
    cache.layout.appendToColumn(cache.layout.singleColumn.column1, screenshots);
    cache.layout.appendToColumn(cache.layout.twoColumns.column1, screenshots);

    auto descr = descriptionWidgets(details);
    if (!descr.empty())
    {
        cache.push(descr);
        cache.layout.appendToColumn(cache.layout.singleColumn.column1, descr);

        // for two-columns we need to split the widgets, summary goes to 1st column, everything else to 2nd
        if (descr.front().id() == "summary")
        {
            descr.pop_front();
            cache.layout.twoColumns.column1.push_back("summary");
        }
        cache.layout.appendToColumn(cache.layout.twoColumns.column2, descr);
    }
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

scopes::PreviewWidget PreviewStrategy::build_other_metadata(const PackageDetails &details)
{
    scopes::PreviewWidget widget("other_metadata", "table");
    scopes::VariantArray values {
        scopes::Variant{scopes::VariantArray{scopes::Variant{_("Publisher/Creator")}, scopes::Variant{details.publisher}}},
        scopes::Variant{scopes::VariantArray{scopes::Variant{_("Seller")}, scopes::Variant{details.company_name}}},
        scopes::Variant{scopes::VariantArray{scopes::Variant{_("Website")}, scopes::Variant{details.website}}},
        scopes::Variant{scopes::VariantArray{scopes::Variant{_("Contact")}, scopes::Variant{details.support_url}}},
        scopes::Variant{scopes::VariantArray{scopes::Variant{_("License")}, scopes::Variant{details.license}}},
    };
    widget.add_attribute_value("values", scopes::Variant(values));
    return widget;
}

scopes::PreviewWidget PreviewStrategy::build_updates_table(const PackageDetails& details)
{
    scopes::PreviewWidget widget("updates_table", "table");
    widget.add_attribute_value("title", scopes::Variant{_("Updates")});
    scopes::VariantArray values {
        scopes::Variant{scopes::VariantArray{scopes::Variant{_("Version number")}, scopes::Variant{details.version}}},
        scopes::Variant{scopes::VariantArray{scopes::Variant{_("Last updated")}, scopes::Variant{details.last_updated.formatted()}}},
        scopes::Variant{scopes::VariantArray{scopes::Variant{_("First released")}, scopes::Variant{details.date_published.formatted()}}},
        scopes::Variant{scopes::VariantArray{scopes::Variant{_("Size")}, scopes::Variant{click::Formatter::human_readable_filesize(details.binary_filesize)}}},
    };
    widget.add_attribute_value("values", scopes::Variant(values));
    return widget;
}

std::string PreviewStrategy::build_whats_new(const PackageDetails& details)
{
    std::stringstream b;
    b << _("Version") << ": " << details.version << std::endl;
    b << details.changelog;
    return b.str();
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
void PreviewStrategy::populateDetails(std::function<void(const click::PackageDetails& details)> details_callback,
                              std::function<void(const click::ReviewList&,
                                                    click::Reviews::Error)> reviews_callback, bool force_cache)
{

    std::string app_name = get_string_maybe_null(result["name"]);

    if (app_name.empty()) {
        click::PackageDetails details;
        qDebug() << "in populateDetails(), app_name is empty";
        details.package.title = result.title();
        details.package.icon_url = result.art();
        details.description = get_string_maybe_null(result["description"]);
        details.main_screenshot_url = get_string_maybe_null(result["main_screenshot"]);
        details_callback(details);
        reviews_callback(click::ReviewList(), click::Reviews::Error::NoError);
    } else {
        qDebug() << "in populateDetails(), app_name is:" << app_name.c_str();
        // I think this should not be required when we switch the click::Index over
        // to using the Qt bridge. With that, the qt dependency becomes an implementation detail
        // and code using it does not need to worry about threading/event loop topics.
        run_under_qt([this, details_callback, reviews_callback, app_name, force_cache]()
            {
                index_operation = index->get_details(app_name, [this, app_name, details_callback, reviews_callback, force_cache](PackageDetails details, click::Index::Error error){
                    if(error == click::Index::Error::NoError) {
                        qDebug() << "Got details:" << app_name.c_str();
                        details_callback(details);
                    } else {
                        qDebug() << "Error getting details for:" << app_name.c_str();
                        click::PackageDetails details;
                        details.package.title = result.title();
                        details.package.icon_url = result.art();
                        details.description = get_string_maybe_null(result["description"]);
                        details.main_screenshot_url = get_string_maybe_null(result["main_screenshot"]);
                        details_callback(details);
                    }
                    reviews_operation = reviews->fetch_reviews(app_name,
                                                               reviews_callback,
                                                               force_cache);
                }, force_cache);
            });
    }
}

scopes::PreviewWidgetList PreviewStrategy::screenshotsWidgets(const click::PackageDetails& details)
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
    return widgets;
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

    if (result.contains("price") && result.contains("rating"))
    {
        // Add the price and rating as attributes.
        bool purchased = result["purchased"].get_bool();
        std::string price_area{""};

        if (details.package.price == 0.00f)
        {
            price_area = _("FREE");
        }
        else if (purchased)
        {
            price_area = _("✔ PURCHASED");
        }
        else
        {
            price_area = result["formatted_price"].get_string();
        }
        scopes::VariantBuilder builder;
        builder.add_tuple({
                {"value", scopes::Variant(price_area)},
            });
        builder.add_tuple({
                {"value", scopes::Variant("")},
            });
        builder.add_tuple({
                {"value", result["rating"]},
            });
        builder.add_tuple({
                {"value", scopes::Variant("")},
            });
        header.add_attribute_value("attributes", builder.end());
    }

    widgets.push_back(header);

    qDebug() << "Pushed widgets for package:" << QString::fromStdString(details.package.name);
    return widgets;
}

scopes::PreviewWidgetList PreviewStrategy::descriptionWidgets(const click::PackageDetails& details)
{
    scopes::PreviewWidgetList widgets;
    if (!details.description.empty())
    {
        scopes::PreviewWidget summary("summary", "text");
        summary.add_attribute_value("title", scopes::Variant(_("Info")));
        if (result.contains("description") && !result["description"].get_string().empty())
        {
            summary.add_attribute_value("text", scopes::Variant(result["description"].get_string()));
        }
        else
        {
            summary.add_attribute_value("text", scopes::Variant(details.description));
        }
        widgets.push_back(summary);
    }

    if (!details.download_url.empty())
    {
        widgets.push_back(build_other_metadata(details));
        widgets.push_back(build_updates_table(details));

        scopes::PreviewWidget whats_new("whats_new", "text");
        whats_new.add_attribute_value("title", scopes::Variant(_("What's new")));
        whats_new.add_attribute_value("text", scopes::Variant(build_whats_new(details)));
        widgets.push_back(whats_new);
    }
    return widgets;
}

scopes::PreviewWidgetList PreviewStrategy::reviewsWidgets(const click::ReviewList& reviewlist)
{
    scopes::PreviewWidgetList widgets;

    scopes::PreviewWidget rating("reviews", "reviews");
    scopes::VariantBuilder builder;

    if (reviewlist.size() > 0) {
        scopes::PreviewWidget title("reviews_title", "text");
        title.add_attribute_value("title", scopes::Variant(_("Reviews")));
        widgets.push_back(title);

        for (const auto& kv : reviewlist) {
            builder.add_tuple({
                    {"rating", scopes::Variant(kv.rating)},
                    {"author", scopes::Variant(kv.reviewer_name)},
                    {"review", scopes::Variant(kv.review_text)}
                });
        }
        rating.add_attribute_value("reviews", builder.end());
        widgets.push_back(rating);
    }

    return widgets;
}

scopes::PreviewWidgetList PreviewStrategy::downloadErrorWidgets()
{
    return errorWidgets(scopes::Variant(_("Download Error")),
                        scopes::Variant(_("Download or install failed. Please try again.")),
                        scopes::Variant(click::Preview::Actions::SHOW_UNINSTALLED),
                        scopes::Variant(_("Close")));
}

scopes::PreviewWidgetList PreviewStrategy::loginErrorWidgets(const std::string& download_url, const std::string& download_sha512)
{
    auto widgets = errorWidgets(scopes::Variant(_("Login Error")),
                                scopes::Variant(_("Please log in to your Ubuntu One account.")),
                                scopes::Variant(click::Preview::Actions::INSTALL_CLICK),
                                scopes::Variant(_("Go to Accounts")));
    auto buttons = widgets.back();
    widgets.pop_back();

    scopes::VariantBuilder builder;
    builder.add_tuple(
        {
            {"id", scopes::Variant(click::Preview::Actions::INSTALL_CLICK)},
            {"label", scopes::Variant(_("Go to Accounts"))},
            {"download_url", scopes::Variant(download_url)},
            {"download_sha512", scopes::Variant(download_sha512)},
        });
    buttons.add_attribute_value("actions", builder.end());
    oa_client.register_account_login_item(buttons,
                                          scopes::OnlineAccountClient::PostLoginAction::ContinueActivation,
                                          scopes::OnlineAccountClient::PostLoginAction::DoNothing);
    widgets.push_back(buttons);
    return widgets;
}

scopes::PreviewWidgetList PreviewStrategy::errorWidgets(const scopes::Variant& title,
                                                const scopes::Variant& summary,
                                                const scopes::Variant& action_id,
                                                const scopes::Variant& action_label,
                                                const scopes::Variant& uri)
{
    scopes::PreviewWidgetList widgets;

    scopes::PreviewWidget header("hdr", "text");
    header.add_attribute_value("title", title);
    header.add_attribute_value("text", summary);
    widgets.push_back(header);

    scopes::PreviewWidget buttons("buttons", "actions");
    scopes::VariantBuilder builder;
    if (uri.is_null())
    {
        builder.add_tuple({ {"id", action_id}, {"label", action_label} });
    }
    else
    {
        builder.add_tuple({ {"id", action_id}, {"label", action_label}, {"uri", uri} });
    }
    buttons.add_attribute_value("actions", builder.end());
    widgets.push_back(buttons);

    return widgets;
}

bool PreviewStrategy::isRefundable()
{
    if (!result.contains("price"))
    {
        return false;
    }

    if (pay_package.isNull())
    {
        return false;
    }

    std::string pkg_name = get_string_maybe_null(result["name"]);
    if (pkg_name.empty())
    {
        return false;
    }

    return pay_package->is_refundable(pkg_name);
}

void PreviewStrategy::invalidateScope(const std::string& scope_id)
{
    run_under_qt([scope_id]() {
            PackageManager::invalidate_results(scope_id);
        });
}

// class DownloadErrorPreview

DownloadErrorPreview::DownloadErrorPreview(const unity::scopes::Result &result)
    : PreviewStrategy(result)
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
                                     const std::string &download_sha512,
                                     const unity::scopes::Result &result,
                                     const QSharedPointer<click::web::Client>& client,
                                     const QSharedPointer<Ubuntu::DownloadManager::Manager>& manager,
                                     std::shared_ptr<click::DepartmentsDb> depts) :
    PreviewStrategy(result, client),
    DepartmentUpdater(depts),
    download_url(download_url),
    download_sha512(download_sha512),
    dm(new DownloadManager(client, manager)),
    depts_db(depts)
{
}

InstallingPreview::~InstallingPreview()
{
}

void InstallingPreview::startLauncherAnimation(const PackageDetails &details)
{
    Launcher l(LAUNCHER_BUSNAME, LAUNCHER_OBJECT_PATH, QDBusConnection::sessionBus());
    l.startInstallation(QString::fromStdString(details.package.title),
                        QString::fromStdString(details.package.icon_url),
                        QString::fromStdString(details.package.name));

}

void InstallingPreview::run(const unity::scopes::PreviewReplyProxy &reply)
{
    qDebug() << "Starting installation" << QString(download_url.c_str()) << QString(download_sha512.c_str());
    std::promise<bool> promise;
    auto future = promise.get_future();
    run_under_qt([this, reply, &promise]() {
            dm->start(download_url, download_sha512, result["name"].get_string(),
                      [this, reply, &promise] (std::string msg, DownloadManager::Error dmerr){
                          switch (dmerr)
                          {
                          case DownloadManager::Error::DownloadInstallError:
                              qWarning() << "Error received from UDM during startDownload:" << msg.c_str();
                              reply->push(downloadErrorWidgets());
                              promise.set_value(false);
                              break;
                          case DownloadManager::Error::CredentialsError:
                              qWarning() << "InstallingPreview got error in getting credentials during startDownload";
                              reply->push(loginErrorWidgets(download_url, download_sha512));
                              promise.set_value(false);
                              break;
                          case DownloadManager::Error::NoError: {
                              std::string object_path = msg;
                              qDebug() << "Successfully created UDM Download.";
                              populateDetails([this, reply, object_path](const PackageDetails &details) {
                                      store_department(details);
                                      pushPackagePreviewWidgets(cachedWidgets, details, progressBarWidget(object_path));
                                      startLauncherAnimation(details);
                                  },
                                  [this, reply, &promise](const ReviewList& reviewlist,
                                                          click::Reviews::Error error) {
                                      if (error == click::Reviews::Error::NoError) {
                                          auto const revs = reviewsWidgets(reviewlist);
                                          cachedWidgets.push(revs);
                                          cachedWidgets.layout.appendToColumn(cachedWidgets.layout.singleColumn.column1, revs);
                                          cachedWidgets.layout.appendToColumn(cachedWidgets.layout.twoColumns.column1, revs);
                                      } else {
                                          qDebug() << "There was an error getting reviews for:" << result["name"].get_string().c_str();
                                      }
                                      cachedWidgets.flush(reply);
                                      promise.set_value(true);
                                  });
                              break;
                          }
                          default:
                              qCritical() << "Unknown error occurred downloading.";
                              promise.set_value(false);
                              break;
                          }
                      });
        });
    future.get();
    reply->finished();
}

scopes::PreviewWidgetList PreviewStrategy::progressBarWidget(const std::string& object_path)
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

std::string InstalledPreview::get_consumer_key()
{
    std::promise<std::string> promise;
    auto future = promise.get_future();
    QSharedPointer<click::CredentialsService> sso;

    qt::core::world::enter_with_task([this, &sso, &promise]() {
            sso.reset(new click::CredentialsService());

            QObject::connect(sso.data(), &click::CredentialsService::credentialsFound,
                    [&promise, &sso](const UbuntuOne::Token& token) {
                    qDebug() << "Credentials found";
                    sso.clear();
                    try { promise.set_value(token.consumerKey().toStdString()); }
                    catch (const std::future_error&) {} // Ignore promise_already_satisfied
                });
            QObject::connect(sso.data(), &click::CredentialsService::credentialsNotFound,
                    [&promise, &sso]() {
                    qDebug() << "No credentials found";
                    sso.clear();
                    try { promise.set_value(""); }
                    catch (const std::future_error&) {} // Ignore promise_already_satisfied
                    });

            sso->getCredentials();
            qDebug() << "getCredentials finished";
        });
    return future.get();
}

scopes::PreviewWidget InstalledPreview::createRatingWidget(const click::Review& review) const
{
    scopes::PreviewWidget rating("rating", "rating-input");

    if (review.id != 0) {
        qDebug() << "Review for current user already exists, review id:" << review.id;
        rating = scopes::PreviewWidget(std::to_string(review.id), "rating-edit"); // pass review id via widget id
        rating.add_attribute_value("review", scopes::Variant(review.review_text));
        rating.add_attribute_value("rating", scopes::Variant(review.rating));
        rating.add_attribute_value("author", scopes::Variant(review.reviewer_name));
    }

    return rating;
}

void InstalledPreview::run(unity::scopes::PreviewReplyProxy const& reply)
{
    const bool force_cache = (metadata.internet_connectivity() == scopes::QueryMetadata::ConnectivityStatus::Disconnected);
    qDebug() << "preview, force_cache=" << force_cache << ", conn status=" << (int)metadata.internet_connectivity();

    // Check if the user is submitting a rating, so we can submit it.
    Review review;
    review.rating = 0;
    std::string widget_id;
    // We use a try/catch here, as scope_data() can be a dict, but not have
    // the values we need, which will result in an exception thrown.
    try {
        auto metadict = metadata.scope_data().get_dict();
        review.rating = metadict["rating"].get_int();
        review.review_text = metadict["review"].get_string();
        widget_id = metadict["widget_id"].get_string();
    } catch(...) {
        // Do nothing as we are not submitting a review.
    }

    auto userid = get_consumer_key();

    //
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

                    // Fill in required data about the package being reviewed.
                    review.package_name = found_manifest.name;
                    review.package_version = found_manifest.version;

                    if (error != click::InterfaceError::NoError) {
                        qDebug() << "There was an error getting the manifest for:" << app_name.c_str();
                    }
                    manifest_promise.set_value(found_manifest);
            });
        });
        manifest = manifest_future.get();
        if (review.rating > 0) {
            std::promise<bool> submit_promise;
            std::future<bool> submit_future = submit_promise.get_future();
            qt::core::world::enter_with_task([this, review, &submit_promise, widget_id]() mutable {
                    if (widget_id == "rating") {
                        submit_operation = reviews->submit_review(review,
                                                              [&submit_promise](click::Reviews::Error){
                                                                  // TODO: Need to handle errors properly.
                                                                  submit_promise.set_value(true);
                                                              });

                    } else {
                        try {
                            review.id = std::stoul(widget_id);
                            qDebug() << "Updating review" << review.id << "with '" << QString::fromStdString(review.review_text) << "'";
                            submit_operation = reviews->edit_review(review,
                                                                [&submit_promise](click::Reviews::Error){
                                                                    // TODO: Need to handle errors properly.
                                                                    submit_promise.set_value(true);
                                                                });
                        } catch (const std::exception &e) {
                            qWarning() << "Failed to update review:" << QString::fromStdString(e.what()) << " review widget:" << QString::fromStdString(widget_id);
                            submit_promise.set_value(false);
                        }
                }
            });
            submit_future.get();
        }
    }
    getApplicationUri(manifest, [this, reply, manifest, app_name, &review, userid, force_cache](const std::string& uri) {
            populateDetails([this, reply, uri, manifest, app_name](const PackageDetails &details){
                cachedDetails = details;
                store_department(details);
                pushPackagePreviewWidgets(cachedWidgets, details, createButtons(uri, manifest));
            },
            [this, reply, &review, manifest, userid](const ReviewList& reviewlist,
                                                     click::Reviews::Error error) {
                auto reviews = bring_to_front(reviewlist, userid);
                if (manifest.removable && !cachedDetails.download_url.empty()) {
                    scopes::PreviewWidgetList review_input;
                    bool has_reviewed = reviews.size() > 0 && reviews.front().reviewer_username == userid;

                    Review existing_review;
                    existing_review.id = 0;
                    if (has_reviewed) {
                        existing_review = reviews.front();
                        reviews.pop_front();
                    }
                    review_input.push_back(createRatingWidget(existing_review));
                    cachedWidgets.push(review_input);
                    cachedWidgets.layout.appendToColumn(cachedWidgets.layout.singleColumn.column1, review_input);
                    cachedWidgets.layout.appendToColumn(cachedWidgets.layout.twoColumns.column1, review_input);
                }

                if (error == click::Reviews::Error::NoError) {
                    auto const revs = reviewsWidgets(reviews);
                    cachedWidgets.push(revs);
                    cachedWidgets.layout.appendToColumn(cachedWidgets.layout.singleColumn.column1, revs);
                    cachedWidgets.layout.appendToColumn(cachedWidgets.layout.twoColumns.column1, revs);
                } else {
                    qDebug() << "There was an error getting reviews for:" << result["name"].get_string().c_str();
                }
                cachedWidgets.flush(reply);
                reply->finished();
        }, force_cache);
    });
}

scopes::PreviewWidgetList InstalledPreview::createButtons(const std::string& uri,
                                                          const Manifest& manifest)
{
    scopes::PreviewWidgetList widgets;
    scopes::PreviewWidget buttons("buttons", "actions");
    scopes::VariantBuilder builder;

    std::string open_label = _("Open");

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
        auto price = result.contains("price") ? result["price"].get_double() : 0.00f;
        if (price > 0.00f && isRefundable()) {
            builder.add_tuple({
                {"id", scopes::Variant(click::Preview::Actions::CANCEL_PURCHASE_INSTALLED)},
                {"label", scopes::Variant(_("Cancel Purchase"))}
            });
        } else {
            builder.add_tuple({
                {"id", scopes::Variant(click::Preview::Actions::UNINSTALL_CLICK)},
                {"label", scopes::Variant(_("Uninstall"))}
            });
        }
    }
    if (!uri.empty() || manifest.removable) {
        buttons.add_attribute_value("actions", builder.end());
        widgets.push_back(buttons);
    }
    return widgets;
}

void InstalledPreview::getApplicationUri(const Manifest& manifest, std::function<void(const std::string&)> callback)
{
    QString app_url = QString::fromStdString(result.uri());

    // asynchronously get application uri based on app name, if the uri is not application://.
    // this can happen if the app was just installed and we have its http uri from the Result.
    if (!app_url.startsWith("application:///")) {
        const std::string name = result["name"].get_string();

        if (manifest.has_any_apps()) {
            qt::core::world::enter_with_task([this, name, callback] ()
            {
                click::Interface().get_dotdesktop_filename(name,
                                              [callback, name] (std::string val, click::InterfaceError error) {
                                              std::string uri;
                                              if (error == click::InterfaceError::NoError) {
                                                  uri = "application:///" + val;
                                              } else {
                                                  qWarning() << "Can't get .desktop filename for"
                                                             << QString::fromStdString(name);
                                              }
                                              callback(uri);
                                     }
                    );
            });
        } else {
            if (manifest.has_any_scopes()) {
                unity::scopes::CannedQuery cq(manifest.first_scope_id);
                auto scope_uri = cq.to_uri();
                qDebug() << "Found uri for scope" << QString::fromStdString(manifest.first_scope_id)
                         << "-" << QString::fromStdString(scope_uri);
                callback(scope_uri);
            }
        }
    } else {
        callback(result.uri());
    }
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

// class PurchasingPreview

PurchasingPreview::PurchasingPreview(const unity::scopes::Result& result,
                                     const QSharedPointer<click::web::Client>& client)
    : PreviewStrategy(result, client)
{
}

PurchasingPreview::~PurchasingPreview()
{
}

void PurchasingPreview::run(unity::scopes::PreviewReplyProxy const& reply)
{
    populateDetails([this, reply](const PackageDetails &details){
            reply->push(purchasingWidgets(details));
        },
        [this, reply](const click::ReviewList&, click::Reviews::Error) {
            reply->finished();
        });
}

scopes::PreviewWidgetList PurchasingPreview::purchasingWidgets(const PackageDetails &/*details*/)
{
    scopes::PreviewWidgetList widgets;
    return widgets;
}

// class CancelPurchasePreview

CancelPurchasePreview::CancelPurchasePreview(const unity::scopes::Result& result, bool installed)
    : PreviewStrategy(result), installed(installed)
{
}

CancelPurchasePreview::~CancelPurchasePreview()
{
}

scopes::PreviewWidgetList CancelPurchasePreview::build_widgets()
{
    scopes::PreviewWidgetList widgets;

    scopes::PreviewWidget confirmation("confirmation", "text");

    std::string title = result["title"].get_string();
    // TRANSLATORS: Do NOT translate ${title} here.
    std::string message =
        _("Are you sure you want to cancel the purchase of '${title}'? The app will be uninstalled.");

    boost::replace_first(message, "${title}", title);
    confirmation.add_attribute_value("text", scopes::Variant(message));
    widgets.push_back(confirmation);

    scopes::PreviewWidget buttons("buttons", "actions");
    scopes::VariantBuilder builder;

    auto action_no = installed ? click::Preview::Actions::SHOW_INSTALLED
                               : click::Preview::Actions::SHOW_UNINSTALLED;
    auto action_yes = installed
        ? click::Preview::Actions::CONFIRM_CANCEL_PURCHASE_INSTALLED
        : click::Preview::Actions::CONFIRM_CANCEL_PURCHASE_UNINSTALLED;

    builder.add_tuple({
       {"id", scopes::Variant(action_no)},
       {"label", scopes::Variant(_("Go Back"))}
    });
    builder.add_tuple({
       {"id", scopes::Variant(action_yes)},
       {"label", scopes::Variant(_("Continue"))}
    });

    buttons.add_attribute_value("actions", builder.end());
    widgets.push_back(buttons);

    scopes::PreviewWidget policy("policy", "text");
    policy.add_attribute_value("title", scopes::Variant{_("Returns and cancellation policy")});
    policy.add_attribute_value("text", scopes::Variant{
        _("When purchasing an app in the Ubuntu Store, you can cancel the charge within 15 minutes "
          "after installation. If the cancel period has passed, we recommend contacting the app "
          "developer directly for a refund.\n"
          "You can find the developer’s contact information listed on the app’s preview page in the "
          "Ubuntu Store.\n"
          "Keep in mind that you cannot cancel the purchasing process of an app more than once.")});
    widgets.push_back(policy);

    return widgets;
}

void CancelPurchasePreview::run(unity::scopes::PreviewReplyProxy const& reply)
{
    // NOTE: no need to populateDetails() here.
    reply->push(build_widgets());
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
    const bool force_cache = (metadata.internet_connectivity() == scopes::QueryMetadata::ConnectivityStatus::Disconnected);
    qDebug() << "preview, force_cache=" << force_cache << ", conn status=" << (int)metadata.internet_connectivity();

    qDebug() << "in UninstalledPreview::run, about to populate details";
    populateDetails([this, reply](const PackageDetails &details){
            store_department(details);
            found_details = details;
        },
        [this, reply](const ReviewList& reviewlist,
                      click::Reviews::Error reviewserror) {
            std::string app_name = result["name"].get_string();
            dm->get_progress(app_name,
                             [this, reply, reviewlist, reviewserror](std::string object_path){
                found_object_path = object_path;
                scopes::PreviewWidgetList button_widgets;
                if(found_object_path.empty()) {
                    button_widgets = uninstalledActionButtonWidgets(found_details);
                } else {
                    button_widgets = progressBarWidget(found_object_path);
                }
                qDebug() << "Pushed button action widgets.";
                pushPackagePreviewWidgets(cachedWidgets, found_details, button_widgets);
                qDebug() << "Pushed package details widgets.";
                if (reviewserror == click::Reviews::Error::NoError) {
                    qDebug() << "Pushing reviews widgets.";
                    auto const revs = reviewsWidgets(reviewlist);
                    cachedWidgets.push(revs);
                    cachedWidgets.layout.appendToColumn(cachedWidgets.layout.singleColumn.column1, revs);
                    cachedWidgets.layout.appendToColumn(cachedWidgets.layout.twoColumns.column1, revs);
                } else {
                    qDebug() << "There was an error getting reviews for:" << result["name"].get_string().c_str();
                }
                cachedWidgets.flush(reply);
                reply->finished();
                qDebug() << "---------- Finished reply for:" << result["name"].get_string().c_str();
            });
        }, force_cache);
}

scopes::PreviewWidgetList UninstalledPreview::uninstalledActionButtonWidgets(const PackageDetails &details)
{
    scopes::PreviewWidgetList widgets;
    auto price = result["price"].get_double();

    if (price > double(0.00)
        && result["purchased"].get_bool() == false) {
        scopes::PreviewWidget payments("purchase", "payments");
        scopes::VariantMap tuple;
        tuple["currency"] = result["currency_symbol"].get_string();
        tuple["price"] = scopes::Variant(price);
        tuple["store_item_id"] = details.package.name;
        tuple["download_url"] = details.download_url;
        tuple["download_sha512"] = details.download_sha512;
        payments.add_attribute_value("source", scopes::Variant(tuple));
        widgets.push_back(payments);
    } else {
        scopes::PreviewWidget buttons("buttons", "actions");
        scopes::VariantBuilder builder;
        builder.add_tuple(
            {
                {"id", scopes::Variant(click::Preview::Actions::INSTALL_CLICK)},
                {"label", scopes::Variant(_("Install"))},
                {"download_url", scopes::Variant(details.download_url)},
                {"download_sha512", scopes::Variant(details.download_sha512)},
            });
        if (isRefundable()) {
            builder.add_tuple(
                {
                    {"id", scopes::Variant(click::Preview::Actions::CANCEL_PURCHASE_UNINSTALLED)},
                    {"label", scopes::Variant(_("Cancel Purchase"))},
                });
        }
        buttons.add_attribute_value("actions", builder.end());
        oa_client.register_account_login_item(buttons,
                                              scopes::OnlineAccountClient::PostLoginAction::ContinueActivation,
                                              scopes::OnlineAccountClient::PostLoginAction::DoNothing);
        widgets.push_back(buttons);
    }

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


// class CancellingPurchasePreview : public UninstallingPreview

CancellingPurchasePreview::CancellingPurchasePreview(const unity::scopes::Result& result,
                                                     const unity::scopes::ActionMetadata& metadata,
                                                     const QSharedPointer<click::web::Client>& client,
                                                     const QSharedPointer<pay::Package>& ppackage,
                                                     const QSharedPointer<Ubuntu::DownloadManager::Manager>& manager,
                                                     bool installed)
    : UninstallingPreview(result, metadata, client, manager, ppackage),
      installed(installed)
{
}

CancellingPurchasePreview::~CancellingPurchasePreview()
{
}

void CancellingPurchasePreview::run(unity::scopes::PreviewReplyProxy const& reply)
{
    qDebug() << "in CancellingPurchasePreview::run, calling cancel_purchase";
    cancel_purchase();
    qDebug() << "in CancellingPurchasePreview::run, calling next ::run()";
    if (installed) {
        UninstallingPreview::run(reply);
    } else {
        UninstalledPreview::run(reply);
    }
}

void CancellingPurchasePreview::cancel_purchase()
{
    auto package_name = result["name"].get_string();
    qDebug() << "Will cancel the purchase of:" << package_name.c_str();

    std::promise<bool> refund_promise;
    std::future<bool> refund_future = refund_promise.get_future();

    run_under_qt([this, &refund_promise, package_name]() {
        qDebug() << "Calling refund for:" << package_name.c_str();
        auto ret = pay_package->refund(package_name);
        qDebug() << "Refund returned:" << ret;
        refund_promise.set_value(ret);
    });
    bool finished = refund_future.get();
    qDebug() << "Finished refund:" << finished;
    if (finished) {
        // Reset the purchased flag.
        result["purchased"] = false;
        invalidateScope(STORE_SCOPE_ID.toUtf8().data());
    }
}


} // namespace click
