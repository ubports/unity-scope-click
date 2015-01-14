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

#include <click/application.h>
#include <click/interface.h>
#include "preview.h"
#include <click/qtbridge.h>
#include <click/download-manager.h>
#include <click/launcher.h>
#include <click/dbus_constants.h>
#include <click/departments-db.h>
#include <click/utils.h>

#include <boost/algorithm/string/replace.hpp>

#include <unity/UnityExceptions.h>
#include <unity/scopes/CannedQuery.h>
#include <unity/scopes/PreviewReply.h>
#include <unity/scopes/Variant.h>
#include <unity/scopes/VariantBuilder.h>

#include <QDebug>

#include <functional>
#include <iostream>
#include <sstream>

#include <click/click-i18n.h>

namespace click {

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
                 const unity::scopes::ActionMetadata& metadata)
    : PreviewQueryBase(result, metadata), result(result), metadata(metadata)
{
}

Preview::~Preview()
{
}

void Preview::choose_strategy(const QSharedPointer<web::Client> &client,
                              const QSharedPointer<click::network::AccessManager>& nam,
                              std::shared_ptr<click::DepartmentsDb> depts)
{
    strategy.reset(build_strategy(result, metadata, client, nam, depts));
}

PreviewStrategy* Preview::build_installing(const std::string& download_url,
                                           const std::string& download_sha512,
                                           const unity::scopes::Result& result,
                                           const QSharedPointer<click::web::Client>& client,
                                           const QSharedPointer<click::network::AccessManager>& nam,
                                           std::shared_ptr<click::DepartmentsDb> depts)
{
    return new InstallingPreview(download_url, download_sha512, result, client, nam, depts);
}


PreviewStrategy* Preview::build_strategy(const unity::scopes::Result &result,
                                          const unity::scopes::ActionMetadata &metadata,
                                          const QSharedPointer<web::Client> &client,
                                          const QSharedPointer<click::network::AccessManager>& nam,
                                          std::shared_ptr<click::DepartmentsDb> depts)
{
    if (metadata.scope_data().which() != scopes::Variant::Type::Null) {
        auto metadict = metadata.scope_data().get_dict();

        if (metadict.count(click::Preview::Actions::DOWNLOAD_FAILED) != 0) {
            return new DownloadErrorPreview(result);
        } else if (metadict.count(click::Preview::Actions::DOWNLOAD_COMPLETED) != 0  ||
                   metadict.count(click::Preview::Actions::CLOSE_PREVIEW) != 0) {
            qDebug() << "in Scope::preview(), metadata has download_completed="
                     << metadict.count(click::Preview::Actions::DOWNLOAD_COMPLETED)
                     << " and close_preview="
                     << metadict.count(click::Preview::Actions::CLOSE_PREVIEW);

            return new InstalledPreview(result, metadata, client, depts);
        } else if (metadict.count("action_id") != 0  && metadict.count("download_url") != 0) {
            std::string action_id = metadict["action_id"].get_string();
            std::string download_url = metadict["download_url"].get_string();
            std::string download_sha512 = metadict["download_sha512"].get_string();
            if (action_id == click::Preview::Actions::INSTALL_CLICK) {
                return build_installing(download_url, download_sha512, result, client, nam, depts);
            } else {
                qWarning() << "unexpected action id " << QString::fromStdString(action_id)
                           << " given with download_url" << QString::fromStdString(download_url);
                return new UninstalledPreview(result, client, depts, nam);
            }
        } else if (metadict.count(click::Preview::Actions::UNINSTALL_CLICK) != 0) {
            return new UninstallConfirmationPreview(result);
        } else if (metadict.count(click::Preview::Actions::CONFIRM_UNINSTALL) != 0) {
            return new UninstallingPreview(result, client, nam);
        } else if (metadict.count(click::Preview::Actions::RATED) != 0) {
            return new InstalledPreview(result, metadata, client, depts);
        } else {
            qWarning() << "preview() called with unexpected metadata. returning uninstalled preview";
            return new UninstalledPreview(result, client, depts, nam);
        }
    } else {
        // metadata.scope_data() is Null, so we return an appropriate "default" preview:
        if (result.uri().find("scope://") == 0)
        {
            return new InstalledScopePreview(result);
        }
        if (result["installed"].get_bool() == true) {
            return new InstalledPreview(result, metadata, client, depts);
        } else {
            return new UninstalledPreview(result, client, depts, nam);
        }
    }

}

void Preview::cancelled()
{
    strategy->cancelled();
}

void Preview::run(const unity::scopes::PreviewReplyProxy &reply)
{
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

void PreviewStrategy::pushPackagePreviewWidgets(const unity::scopes::PreviewReplyProxy &reply,
                                                const PackageDetails &details,
                                                const scopes::PreviewWidgetList& button_area_widgets)
{
    reply->push(headerWidgets(details));
    reply->push(button_area_widgets);
    reply->push(screenshotsWidgets(details));
    reply->push(descriptionWidgets(details));
}

PreviewStrategy::~PreviewStrategy()
{
}

void PreviewStrategy::cancelled()
{
    index_operation.cancel();
    reviews_operation.cancel();
    submit_operation.cancel();
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

// TODO: error handling - once get_details provides errors, we can
// return them from populateDetails and check them in the calling code
// to decide whether to show error widgets. see bug LP: #1289541
void PreviewStrategy::populateDetails(std::function<void(const click::PackageDetails& details)> details_callback,
                              std::function<void(const click::ReviewList&,
                                                    click::Reviews::Error)> reviews_callback)
{

    std::string app_name = result["name"].get_string();

    if (app_name.empty()) {
        click::PackageDetails details;
        qDebug() << "in populateDetails(), app_name is empty";
        details.package.title = result.title();
        details.package.icon_url = result.art();
        details.description = result["description"].get_string();
        details.main_screenshot_url = result["main_screenshot"].get_string();
        details_callback(details);
        reviews_callback(click::ReviewList(), click::Reviews::Error::NoError);
    } else {
        qDebug() << "in populateDetails(), app_name is:" << app_name.c_str();
        // I think this should not be required when we switch the click::Index over
        // to using the Qt bridge. With that, the qt dependency becomes an implementation detail
        // and code using it does not need to worry about threading/event loop topics.
        qt::core::world::enter_with_task([this, details_callback, reviews_callback, app_name]()
            {
                index_operation = index->get_details(app_name, [this, app_name, details_callback, reviews_callback](PackageDetails details, click::Index::Error error){
                    if(error == click::Index::Error::NoError) {
                        qDebug() << "Got details:" << app_name.c_str();
                        details_callback(details);
                    } else {
                        qDebug() << "Error getting details for:" << app_name.c_str();
                        click::PackageDetails details;
                        details.package.title = result.title();
                        details.package.icon_url = result.art();
                        details.description = result["description"].get_string();
                        details.main_screenshot_url = result["main_screenshot"].get_string();
                        details_callback(details);
                    }
                    reviews_operation = reviews->fetch_reviews(app_name,
                                                               reviews_callback);
                });
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

    if (result.contains("price_area") && result.contains("rating"))
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
            price_area =  result["price_area"].get_string();
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

    scopes::PreviewWidget rating("summary", "reviews");
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
                        scopes::Variant(click::Preview::Actions::CLOSE_PREVIEW), // TODO see bug LP: #1289434
                        scopes::Variant(_("Close")));
}

scopes::PreviewWidgetList PreviewStrategy::loginErrorWidgets(const PackageDetails& details)
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
            {"download_url", scopes::Variant(details.download_url)},
            {"download_sha512", scopes::Variant(details.download_sha512)},
        });
    buttons.add_attribute_value("actions", builder.end());
    oa_client.register_account_login_item(buttons,
                                          scopes::OnlineAccountClient::PostLoginAction::ContinueActivation,
                                          scopes::OnlineAccountClient::PostLoginAction::DoNothing);
    widgets.push_back(buttons);
    return widgets;
}

scopes::PreviewWidgetList PreviewStrategy::errorWidgets(const scopes::Variant& title,
                                                const scopes::Variant& subtitle,
                                                const scopes::Variant& action_id,
                                                const scopes::Variant& action_label,
                                                const scopes::Variant& uri)
{
    scopes::PreviewWidgetList widgets;

    scopes::PreviewWidget header("hdr", "header");
    header.add_attribute_value("title", title);
    header.add_attribute_value("subtitle", subtitle);
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
                                     const QSharedPointer<click::network::AccessManager> &nam,
                                     std::shared_ptr<click::DepartmentsDb> depts)
    : PreviewStrategy(result, client), DepartmentUpdater(depts),
      download_url(download_url),
      download_sha512(download_sha512),
      downloader(new click::Downloader(nam)),
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
    downloader->startDownload(download_url, download_sha512, result["name"].get_string(),
            [this, reply] (std::pair<std::string, click::InstallError> rc){
              // NOTE: details not needed by fooErrorWidgets, so no need to populateDetails():
              bool login_error = false;
              switch (rc.second)
              {
              case InstallError::DownloadInstallError:
                  qWarning() << "Error received from UDM during startDownload:" << rc.first.c_str();
                  reply->push(downloadErrorWidgets());
                  return;
              case InstallError::CredentialsError:
                  qWarning() << "InstallingPreview got error in getting credentials during startDownload";
                  login_error = true;
              default:
                  std::string object_path = rc.first;
                  qDebug() << "Successfully created UDM Download.";
                  populateDetails([this, reply, object_path, login_error](const PackageDetails &details) {
                          store_department(details);
                          if (login_error) {
                              reply->push(loginErrorWidgets(details));
                          } else {
                              pushPackagePreviewWidgets(reply, details, progressBarWidget(object_path));
                              startLauncherAnimation(details);
                          }
                      },
                      [this, reply, login_error](const ReviewList& reviewlist,
                                    click::Reviews::Error error) {
                          if (!login_error) {
                              if (error == click::Reviews::Error::NoError) {
                                  reply->push(reviewsWidgets(reviewlist));
                              } else {
                                  qDebug() << "There was an error getting reviews for:" << result["name"].get_string().c_str();
                              }
                          }
                          reply->finished();
                      });
                  break;
              }
            });
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
                                   const std::shared_ptr<click::DepartmentsDb>& depts)
    : PreviewStrategy(result, client),
      DepartmentUpdater(depts),
      metadata(metadata)
{
}

InstalledPreview::~InstalledPreview()
{
}

void InstalledPreview::run(unity::scopes::PreviewReplyProxy const& reply)
{
    // Check if the user is submitting a rating, so we can submit it.
    Review review;
    review.rating = 0;
    // We use a try/catch here, as scope_data() can be a dict, but not have
    // the values we need, which will result in an exception thrown. 
    try {
        auto metadict = metadata.scope_data().get_dict();
        review.rating = metadict["rating"].get_int();
        review.review_text = metadict["review"].get_string();
    } catch(...) {
        // Do nothing as we are not submitting a review.
    }

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
            qt::core::world::enter_with_task([this, review, &submit_promise]() {
                    QSharedPointer<click::CredentialsService> sso(new click::CredentialsService());
                    client->setCredentialsService(sso);
                    submit_operation = reviews->submit_review(review,
                                                              [&submit_promise](click::Reviews::Error){
                                                                  // TODO: Need to handle errors properly.
                                                                  submit_promise.set_value(true);
                                                              });
                });
            submit_future.get();
        }
    }
    getApplicationUri(manifest, [this, reply, manifest, app_name, &review](const std::string& uri) {
            populateDetails([this, reply, uri, manifest, app_name, &review](const PackageDetails &details){
                store_department(details);
                pushPackagePreviewWidgets(reply, details, createButtons(uri, manifest));

                if (review.rating == 0 && manifest.removable) {
                    scopes::PreviewWidgetList review_input;
                    scopes::PreviewWidget rating("rating", "rating-input");
                    rating.add_attribute_value("required", scopes::Variant("rating"));
                    review_input.push_back(rating);
                    reply->push(review_input);
                }
            },
            [this, reply](const ReviewList& reviewlist,
                          click::Reviews::Error error) {
                if (error == click::Reviews::Error::NoError) {
                    reply->push(reviewsWidgets(reviewlist));
                } else {
                    qDebug() << "There was an error getting reviews for:" << result["name"].get_string().c_str();
                }
                reply->finished();
        });
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
       {"id", scopes::Variant(click::Preview::Actions::CLOSE_PREVIEW)}, // TODO: see bug LP: #1289434
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

click::Downloader* UninstalledPreview::get_downloader(const QSharedPointer<click::network::AccessManager>& nam)
{
    static auto downloader = new click::Downloader(nam);
    return downloader;
}

UninstalledPreview::UninstalledPreview(const unity::scopes::Result& result,
                                       const QSharedPointer<click::web::Client>& client,
                                       const std::shared_ptr<click::DepartmentsDb>& depts,
                                       const QSharedPointer<click::network::AccessManager>& nam)
    : PreviewStrategy(result, client),
      DepartmentUpdater(depts), nam(nam)
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
            store_department(details);
            found_details = details;
        },
        [this, reply](const ReviewList& reviewlist,
                      click::Reviews::Error reviewserror) {
            std::string app_name = result["name"].get_string();
            get_downloader(nam)->get_download_progress(app_name,
                                              [this, reply, reviewlist, reviewserror](std::string object_path){
                found_object_path = object_path;
                scopes::PreviewWidgetList button_widgets;
                if(found_object_path.empty()) {
                    button_widgets = uninstalledActionButtonWidgets(found_details);
                } else {
                    button_widgets = progressBarWidget(found_object_path);
                }
                pushPackagePreviewWidgets(reply, found_details, button_widgets);
                if (reviewserror == click::Reviews::Error::NoError) {
                    reply->push(reviewsWidgets(reviewlist));
                } else {
                    qDebug() << "There was an error getting reviews for:" << result["name"].get_string().c_str();
                }
                reply->finished();
                qDebug() << "---------- Finished reply for:" << result["name"].get_string().c_str();
            });
        });
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
        // NOTE: No need to connect payments button to online-accounts API
        // here, as pay-ui will take care of any login needs.
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
                                         const QSharedPointer<click::web::Client>& client,
                                         const QSharedPointer<click::network::AccessManager>& nam)
      : UninstalledPreview(result, client, nullptr, nam)
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
