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

#include "application.h"
#include "interface.h"
#include "preview.h"
#include "qtbridge.h"
#include "download-manager.h"

#include <unity/UnityExceptions.h>
#include <unity/scopes/PreviewReply.h>
#include <unity/scopes/Variant.h>
#include <unity/scopes/VariantBuilder.h>

#include <QDebug>

#include <functional>
#include <iostream>
#include <sstream>

#include "interface.h"
#include "click-i18n.h"

namespace click {

// Preview base class

Preview::Preview(const unity::scopes::Result& result,
                 const unity::scopes::ActionMetadata& metadata,
                 const QSharedPointer<click::web::Client>& client,
                 const QSharedPointer<click::network::AccessManager>& nam)
{
    strategy.reset(choose_strategy(result, metadata, client, nam));
}

PreviewStrategy* Preview::choose_strategy(const unity::scopes::Result &result,
                                          const unity::scopes::ActionMetadata &metadata,
                                          const QSharedPointer<web::Client> &client,
                                          const QSharedPointer<click::network::AccessManager>& nam)
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

            return new InstalledPreview(result, metadata, client);
        } else if (metadict.count("action_id") != 0  && metadict.count("download_url") != 0) {
            std::string action_id = metadict["action_id"].get_string();
            std::string download_url = metadict["download_url"].get_string();
            if (action_id == click::Preview::Actions::INSTALL_CLICK) {
                return new InstallingPreview(download_url, result, client, nam);
            } else {
                qWarning() << "unexpected action id " << QString::fromStdString(action_id)
                           << " given with download_url" << QString::fromStdString(download_url);
                return new UninstalledPreview(result, client);
            }
        } else if (metadict.count(click::Preview::Actions::UNINSTALL_CLICK) != 0) {
            return new UninstallConfirmationPreview(result);
        } else if (metadict.count(click::Preview::Actions::CONFIRM_UNINSTALL) != 0) {
            return new UninstallingPreview(result, client);
        } else if (metadict.count(click::Preview::Actions::RATED) != 0) {
            return new InstalledPreview(result, metadata, client);
        } else {
            qWarning() << "preview() called with unexpected metadata. returning uninstalled preview";
            return new UninstalledPreview(result, client);
        }
    } else {
        // metadata.scope_data() is Null, so we return an appropriate "default" preview:
        if (result["installed"].get_bool() == true) {
            return new InstalledPreview(result, metadata, client);
        } else {
            return new UninstalledPreview(result, client);
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
    : result(result)
{
}

PreviewStrategy::PreviewStrategy(const unity::scopes::Result& result,
                 const QSharedPointer<click::web::Client>& client) :
    result(result),
    client(client),
    index(new click::Index(client)),
    reviews(new click::Reviews(client))
{
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

scopes::PreviewWidgetList PreviewStrategy::headerWidgets(const click::PackageDetails& details)
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
    header.add_attribute_value("title", scopes::Variant(details.package.title));
    if (!details.publisher.empty())
    {
        header.add_attribute_value("subtitle", scopes::Variant(details.publisher));
    }
    if (!details.package.icon_url.empty())
        header.add_attribute_value("mascot", scopes::Variant(details.package.icon_url));
    widgets.push_back(header);

    return widgets;
}

scopes::PreviewWidgetList PreviewStrategy::descriptionWidgets(const click::PackageDetails& details)
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

scopes::PreviewWidgetList PreviewStrategy::reviewsWidgets(const click::ReviewList& reviewlist)
{
    scopes::PreviewWidgetList widgets;

    scopes::PreviewWidget rating("summary", "reviews");
    scopes::VariantBuilder builder;

    if (reviewlist.size() > 0) {
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

scopes::PreviewWidgetList PreviewStrategy::loginErrorWidgets()
{
    return errorWidgets(scopes::Variant(_("Login Error")),
                        scopes::Variant(_("Please log in to your Ubuntu One account.")),
                        scopes::Variant(click::Preview::Actions::OPEN_ACCOUNTS),
                        scopes::Variant(_("Go to Accounts")),
                        scopes::Variant("settings:///system/online-accounts"));
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
                                     const unity::scopes::Result &result,
                                     const QSharedPointer<click::web::Client>& client,
                                     const QSharedPointer<click::network::AccessManager> &nam)
    : PreviewStrategy(result, client), download_url(download_url),
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
                                   const unity::scopes::ActionMetadata& metadata,
                                   const QSharedPointer<click::web::Client>& client)
    : PreviewStrategy(result, client),
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

    // Get the removable flag from the click manifest.
    bool removable = false;
    std::promise<bool> manifest_promise;
    std::future<bool> manifest_future = manifest_promise.get_future();
    std::string app_name = result["name"].get_string();
    if (!app_name.empty()) {
        qt::core::world::enter_with_task([&]() {
            click::Interface().get_manifest_for_app(app_name,
                [&](Manifest manifest, ManifestError error) {
                    qDebug() << "Got manifest for:" << app_name.c_str();
                    removable = manifest.removable;

                    // Fill in required data about the package being reviewed.
                    review.package_name = manifest.name;
                    review.package_version = manifest.version;

                    if (error != click::ManifestError::NoError) {
                        qDebug() << "There was an error getting the manifest for:" << app_name.c_str();
                    }
                    manifest_promise.set_value(true);
            });
        });
        manifest_future.get();
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
    getApplicationUri([this, reply, removable, app_name, &review](const std::string& uri) {
            populateDetails([this, reply, uri, removable, app_name, &review](const PackageDetails &details){
                reply->push(headerWidgets(details));
                reply->push(createButtons(uri, removable));
                reply->push(descriptionWidgets(details));

                // Hide the rating widget until #1314117 is fixed.
                if (false && review.rating == 0 && removable) {
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
                                                          bool removable)
{
    scopes::PreviewWidgetList widgets;
    scopes::PreviewWidget buttons("buttons", "actions");
    scopes::VariantBuilder builder;
    if (!uri.empty())
    {
        builder.add_tuple(
        {
            {"id", scopes::Variant(click::Preview::Actions::OPEN_CLICK)},
            {"label", scopes::Variant(_("Open"))},
            {"uri", scopes::Variant(uri)}
        });
    }
    if (removable)
    {
        builder.add_tuple({
            {"id", scopes::Variant(click::Preview::Actions::UNINSTALL_CLICK)},
            {"label", scopes::Variant(_("Uninstall"))}
        });
    }
    if (!uri.empty() || removable) {
        buttons.add_attribute_value("actions", builder.end());
        widgets.push_back(buttons);
    }
    return widgets;
}

void InstalledPreview::getApplicationUri(std::function<void(const std::string&)> callback)
{
    std::string uri;
    QString app_url = QString::fromStdString(result.uri());

    // asynchronously get application uri based on app name, if the uri is not application://.
    // this can happen if the app was just installed and we have its http uri from the Result.
    if (!app_url.startsWith("application:///")) {
        const std::string name = result["name"].get_string();
        auto ft = qt::core::world::enter_with_task([this, name, callback] ()
        {
            click::Interface().get_dotdesktop_filename(name,
                                          [callback] (std::string val, click::ManifestError error) {
                                          std::string uri;
                                          if (error == click::ManifestError::NoError) {
                                              uri = "application:///" + val;
                                          }
                                          callback(uri);
                                 }
                );
        });
    } else {
        uri = app_url.toStdString();
        callback(uri);
    }
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
    header.add_attribute_value("subtitle",
                               scopes::Variant(_("Uninstalling this app will delete all the related information. Are you sure you want to uninstall?"))); // TODO: wording needs review. see bug LP: #1234211
    widgets.push_back(header);

    scopes::PreviewWidget buttons("buttons", "actions");
    scopes::VariantBuilder builder;
    builder.add_tuple({
       {"id", scopes::Variant(click::Preview::Actions::CLOSE_PREVIEW)}, // TODO: see bug LP: #1289434
       {"label", scopes::Variant(_("Not anymore"))}
    });
    builder.add_tuple({
       {"id", scopes::Variant(click::Preview::Actions::CONFIRM_UNINSTALL)},
       {"label", scopes::Variant(_("Yes Uninstall"))}
    });
    buttons.add_attribute_value("actions", builder.end());
    widgets.push_back(buttons);

    reply->push(widgets);
}

// class UninstalledPreview

UninstalledPreview::UninstalledPreview(const unity::scopes::Result& result,
                                       const QSharedPointer<click::web::Client>& client)
    : PreviewStrategy(result, client)
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
}

scopes::PreviewWidgetList UninstalledPreview::uninstalledActionButtonWidgets(const PackageDetails &details)
{
    scopes::PreviewWidgetList widgets;
    scopes::PreviewWidget buttons("buttons", "actions");
    scopes::VariantBuilder builder;
    builder.add_tuple(
        {
            {"id", scopes::Variant(click::Preview::Actions::INSTALL_CLICK)},
            {"label", scopes::Variant(_("Install"))},
            {"download_url", scopes::Variant(details.download_url)}
        });
    buttons.add_attribute_value("actions", builder.end());
    widgets.push_back(buttons);

    return widgets;
}

// class UninstallingPreview : public UninstalledPreview

// TODO: this class should be removed once uninstall() is handled elsewhere.
UninstallingPreview::UninstallingPreview(const unity::scopes::Result& result,
                                         const QSharedPointer<click::web::Client>& client)
    : UninstalledPreview(result, client)
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
