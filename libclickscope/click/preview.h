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

#ifndef CLICKPREVIEW_H
#define CLICKPREVIEW_H

#include <click/index.h>
#include <click/download-manager.h>
#include <click/qtbridge.h>
#include "reviews.h"

#include <click/network_access_manager.h>

#include <unity/scopes/ActionMetadata.h>
#include <unity/scopes/PreviewQueryBase.h>
#include <unity/scopes/PreviewWidget.h>
#include <unity/scopes/Result.h>
#include <unity/scopes/ScopeBase.h>

namespace scopes = unity::scopes;

namespace click {

class PreviewStrategy;

class Preview : public unity::scopes::PreviewQueryBase
{
protected:
    std::unique_ptr<PreviewStrategy> strategy;
    PreviewStrategy* choose_strategy(const unity::scopes::Result& result,
                                     const unity::scopes::ActionMetadata& metadata,
                                     const QSharedPointer<web::Client> &client,
                                     const QSharedPointer<click::network::AccessManager>& nam);
public:
    struct Actions
    {
        Actions() = delete;

        constexpr static const char* INSTALL_CLICK{"install_click"};
        constexpr static const char* BUY_CLICK{"buy_click"};
        constexpr static const char* DOWNLOAD_COMPLETED{"finished"};
        constexpr static const char* DOWNLOAD_FAILED{"failed"};
        constexpr static const char* PURCHASE_SUCCEEDED{"purchase_succeeded"};
        constexpr static const char* PURCHASE_FAILED{"purchase_failed"};
        constexpr static const char* OPEN_CLICK{"open_click"};
        constexpr static const char* PIN_TO_LAUNCHER{"pin_to_launcher"};
        constexpr static const char* UNINSTALL_CLICK{"uninstall_click"};
        constexpr static const char* CONFIRM_UNINSTALL{"confirm_uninstall"};
        constexpr static const char* CLOSE_PREVIEW{"close_preview"};
        constexpr static const char* OPEN_ACCOUNTS{"open_accounts"};
        constexpr static const char* RATED{"rated"};
    };

    Preview(const unity::scopes::Result& result);
    Preview(const unity::scopes::Result& result,
            const unity::scopes::ActionMetadata& metadata,
            const QSharedPointer<click::web::Client>& client,
            const QSharedPointer<click::network::AccessManager>& nam);
    // From unity::scopes::PreviewQuery
    void cancelled() override;
    virtual void run(unity::scopes::PreviewReplyProxy const& reply) override;
};

class PreviewStrategy
{
public:

    PreviewStrategy(const unity::scopes::Result& result);
    PreviewStrategy(const unity::scopes::Result& result,
            const QSharedPointer<click::web::Client>& client);

    virtual ~PreviewStrategy();

    virtual void cancelled();
    virtual void run(unity::scopes::PreviewReplyProxy const& reply) = 0;

protected:
    virtual void populateDetails(std::function<void(const PackageDetails &)> details_callback,
                                 std::function<void(const click::ReviewList&,
                                                    click::Reviews::Error)> reviews_callback);
    virtual scopes::PreviewWidgetList headerWidgets(const PackageDetails &details);
    virtual scopes::PreviewWidgetList descriptionWidgets(const PackageDetails &details);
    virtual scopes::PreviewWidgetList reviewsWidgets(const click::ReviewList &reviewlist);
    virtual scopes::PreviewWidgetList downloadErrorWidgets();
    virtual scopes::PreviewWidgetList loginErrorWidgets();
    virtual scopes::PreviewWidgetList errorWidgets(const scopes::Variant& title,
                                                   const scopes::Variant& subtitle,
                                                   const scopes::Variant& action_id,
                                                   const scopes::Variant& action_label,
                                                   const scopes::Variant& action_uri = scopes::Variant::null());
    scopes::Result result;
    QSharedPointer<click::web::Client> client;
    QSharedPointer<click::Index> index;
    click::web::Cancellable index_operation;
    QSharedPointer<click::Reviews> reviews;
    click::web::Cancellable reviews_operation;
    click::web::Cancellable submit_operation;
};

class DownloadErrorPreview : public PreviewStrategy
{
public:
    DownloadErrorPreview(const unity::scopes::Result& result);

    virtual ~DownloadErrorPreview();

    void run(unity::scopes::PreviewReplyProxy const& reply) override;
};

class InstallingPreview : public PreviewStrategy
{
public:
    InstallingPreview(std::string const& download_url,
                      const unity::scopes::Result& result,
                      const QSharedPointer<click::web::Client>& client,
                      const QSharedPointer<click::network::AccessManager>& nam);

    virtual ~InstallingPreview();

    void run(unity::scopes::PreviewReplyProxy const& reply) override;

protected:
    virtual scopes::PreviewWidgetList progressBarWidget(const std::string& object_path);
    std::string download_url;
    QSharedPointer<click::Downloader> downloader;

};

class InstalledPreview : public PreviewStrategy
{
public:
    InstalledPreview(const unity::scopes::Result& result,
                     const unity::scopes::ActionMetadata& metadata,
                     const QSharedPointer<click::web::Client>& client);

    virtual ~InstalledPreview();

    void run(unity::scopes::PreviewReplyProxy const& reply) override;

protected:
    void getApplicationUri(std::function<void(const std::string&)> callback);

private:
    static scopes::PreviewWidgetList createButtons(const std::string& uri,
                                                   bool removable);
    scopes::ActionMetadata metadata;
};

class PurchasingPreview : public PreviewStrategy
{
public:
    PurchasingPreview(const unity::scopes::Result& result,
                      const QSharedPointer<click::web::Client>& client);
    virtual ~PurchasingPreview();

    void run(unity::scopes::PreviewReplyProxy const& reply) override;

protected:
    virtual scopes::PreviewWidgetList purchasingWidgets(const PackageDetails &);
};

class UninstallConfirmationPreview : public PreviewStrategy
{
public:
    UninstallConfirmationPreview(const unity::scopes::Result& result);

    virtual ~UninstallConfirmationPreview();

    void run(unity::scopes::PreviewReplyProxy const& reply) override;

};

class UninstalledPreview : public PreviewStrategy
{
public:
    UninstalledPreview(const unity::scopes::Result& result,
                       const QSharedPointer<click::web::Client>& client);

    virtual ~UninstalledPreview();

    void run(unity::scopes::PreviewReplyProxy const& reply) override;
protected:
    virtual scopes::PreviewWidgetList uninstalledActionButtonWidgets(const PackageDetails &details);
};

// TODO: this is only necessary to perform uninstall.
// That should be moved to a separate action, and this class removed.
class UninstallingPreview : public UninstalledPreview
{
public:
    UninstallingPreview(const unity::scopes::Result& result,
                        const QSharedPointer<click::web::Client>& client);

    virtual ~UninstallingPreview();

    void run(unity::scopes::PreviewReplyProxy const& reply) override;

protected:
    void uninstall();

};

} // namespace click

#endif
