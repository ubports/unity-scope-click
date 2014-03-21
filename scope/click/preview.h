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

#include "index.h"
#include "network_access_manager.h"
#include "download-manager.h"
#include "qtbridge.h"

#include <unity/scopes/PreviewQueryBase.h>
#include <unity/scopes/PreviewWidget.h>
#include <unity/scopes/Result.h>

namespace scopes = unity::scopes;

namespace click {

class Preview : public unity::scopes::PreviewQueryBase
{
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
    };

    Preview(const unity::scopes::Result& result,
            const QSharedPointer<click::Index>& index);

    virtual ~Preview();

    // From unity::scopes::PreviewQuery
    void cancelled() override;
    virtual void run(unity::scopes::PreviewReplyProxy const& reply) override = 0;

protected:
    virtual void populateDetails();
    virtual scopes::PreviewWidgetList headerWidgets();
    virtual scopes::PreviewWidgetList descriptionWidgets();
    virtual scopes::PreviewWidgetList downloadErrorWidgets();
    virtual scopes::PreviewWidgetList loginErrorWidgets();
    virtual scopes::PreviewWidgetList errorWidgets(const scopes::Variant& title,
                                                   const scopes::Variant& subtitle,
                                                   const scopes::Variant& action_id,
                                                   const scopes::Variant& action_label);
    scopes::Result result;
    QSharedPointer<click::Index> index;
    click::PackageDetails details;
};

class DownloadErrorPreview : public Preview
{
public:
    DownloadErrorPreview(const unity::scopes::Result& result,
                         const QSharedPointer<click::Index>& index);

    virtual ~DownloadErrorPreview();

    void run(unity::scopes::PreviewReplyProxy const& reply) override;
};

class InstallingPreview : public Preview
{
public:
    InstallingPreview(std::string const& download_url,
                      const unity::scopes::Result& result,
                      const QSharedPointer<click::Index>& index,
                      const QSharedPointer<click::network::AccessManager>& nam);

    virtual ~InstallingPreview();

    void run(unity::scopes::PreviewReplyProxy const& reply) override;

protected:
    virtual scopes::PreviewWidgetList progressBarWidget(const std::string& object_path);
    std::string download_url;
    QSharedPointer<click::Downloader> downloader;

};

class InstalledPreview : public Preview
{
public:
    InstalledPreview(const unity::scopes::Result& result,
                     const QSharedPointer<click::Index>& index);

    virtual ~InstalledPreview();

    void run(unity::scopes::PreviewReplyProxy const& reply) override;
protected:
    virtual scopes::PreviewWidgetList installedActionButtonWidgets();
};

class PurchasingPreview : public Preview
{
public:
    PurchasingPreview(const unity::scopes::Result& result,
                      const QSharedPointer<click::Index>& index);
    virtual ~PurchasingPreview();

    void run(unity::scopes::PreviewReplyProxy const& reply) override;

protected:
    virtual scopes::PreviewWidgetList purchasingWidgets();
};

class UninstallConfirmationPreview : public Preview
{
public:
    UninstallConfirmationPreview(const unity::scopes::Result& result,
                                 const QSharedPointer<click::Index>& index);

    virtual ~UninstallConfirmationPreview();

    void run(unity::scopes::PreviewReplyProxy const& reply) override;

};

class UninstalledPreview : public Preview
{
public:
    UninstalledPreview(const unity::scopes::Result& result,
                       const QSharedPointer<click::Index>& index);

    virtual ~UninstalledPreview();

    void run(unity::scopes::PreviewReplyProxy const& reply) override;
protected:
    virtual scopes::PreviewWidgetList uninstalledActionButtonWidgets();
};

// TODO: this is only necessary to perform uninstall.
// That should be moved to a separate action, and this class removed.
class UninstallingPreview : public UninstalledPreview
{
public:
    UninstallingPreview(const unity::scopes::Result& result,
                       const QSharedPointer<click::Index>& index);

    virtual ~UninstallingPreview();

    void run(unity::scopes::PreviewReplyProxy const& reply) override;

protected:
    void uninstall();

};

} // namespace click

#endif
