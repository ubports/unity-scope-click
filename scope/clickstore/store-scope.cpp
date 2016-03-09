/*
 * Copyright (C) 2014-2015 Canonical Ltd.
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

#include <click/qtbridge.h>
#include <click/departments-db.h>
#include <click/department-lookup.h>
#include "store-scope.h"
#include "store-query.h"
#include <click/preview.h>
#include <click/interface.h>
#include <click/scope_activation.h>

#include <QSharedPointer>

#include <click/key_file_locator.h>
#include <click/network_access_manager.h>
#include <click/click-i18n.h>

#include <logging.h>
#include <iostream>

click::Scope::Scope()
{
    nam.reset(new click::network::AccessManager());
    client.reset(new click::web::Client(nam));
    index.reset(new click::Index(client));
    depts.reset(new click::DepartmentLookup());
    highlights.reset(new click::HighlightList());
    pay_package.reset(new pay::Package(client));

    try
    {
        depts_db = click::DepartmentsDb::open(true);
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << "Failed to open departments db: " << e.what() << std::endl;
    }
}

click::Scope::~Scope()
{
}

void click::Scope::start(std::string const&)
{
    setlocale(LC_ALL, "");
    // FIXME: This is wrong, but needed for json-cpp workaround.
    setlocale(LC_MONETARY, "C");
    bindtextdomain(GETTEXT_PACKAGE, GETTEXT_LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    click::Date::setup_system_locale();
}

void click::Scope::run()
{
    static const int zero = 0;
    auto emptyCb = [this]()
    {
        dm.reset(Ubuntu::DownloadManager::Manager::createSessionManager());
    };

    qt::core::world::build_and_run(zero, nullptr, emptyCb);
}

void click::Scope::stop()
{
    qt::core::world::destroy();
}

scopes::SearchQueryBase::UPtr click::Scope::search(unity::scopes::CannedQuery const& q, scopes::SearchMetadata const& metadata)
{
    return scopes::SearchQueryBase::UPtr(new click::Query(q, *index, *depts, depts_db, *highlights, metadata, *pay_package));
}


unity::scopes::PreviewQueryBase::UPtr click::Scope::preview(const unity::scopes::Result& result,
        const unity::scopes::ActionMetadata& metadata) {
    qDebug() << "Scope::preview() called.";
    auto preview = new click::Preview(result, metadata);
    preview->choose_strategy(client, pay_package, dm, depts_db);
    return unity::scopes::PreviewQueryBase::UPtr{preview};
}

unity::scopes::ActivationQueryBase::UPtr click::Scope::perform_action(unity::scopes::Result const& result, unity::scopes::ActionMetadata const& metadata,
        std::string const& widget_id, std::string const& _action_id)
{
    std::string action_id = _action_id;
    qDebug() << "perform_action called with widget_id" << QString::fromStdString(widget_id) << "and action_id:" << QString::fromStdString(action_id);
    auto activation = new ScopeActivation(result, metadata);

    // if the purchase is completed, do the install
    if (action_id == "purchaseCompleted") {
        qDebug() << "Yay, got finished signal";
        qDebug() << "about to get the download_url";
        std::string download_url = metadata.scope_data().get_dict()["download_url"].get_string();
        qDebug() << "the download url is: " << QString::fromStdString(download_url);
        activation->setHint("download_url", unity::scopes::Variant(download_url));
        std::string download_sha512 = metadata.scope_data().get_dict()["download_sha512"].get_string();
        activation->setHint("download_sha512", unity::scopes::Variant(download_sha512));
        activation->setHint("action_id", unity::scopes::Variant(click::Preview::Actions::INSTALL_CLICK));
        activation->setHint("purchased", unity::scopes::Variant(true));
        qDebug() << "returning ShowPreview";
        activation->setStatus(unity::scopes::ActivationResponse::Status::ShowPreview);
    } else if (action_id == "purchaseCancelled") {
        qDebug() << "Purchase was cancelled, refreshing scope results.";
        click::DownloadErrorPreview strategy{result};
        strategy.invalidateScope(STORE_SCOPE_ID.toUtf8().data());
    } else if (action_id == "purchaseError") {
        activation->setHint(click::Preview::Actions::DOWNLOAD_FAILED, unity::scopes::Variant(true));
        activation->setStatus(unity::scopes::ActivationResponse::Status::ShowPreview);
    } else if (action_id == click::Preview::Actions::INSTALL_CLICK) {
        qDebug() << "about to get the download_url";
        std::string download_url = metadata.scope_data().get_dict()["download_url"].get_string();
        qDebug() << "the download url is: " << QString::fromStdString(download_url);
        activation->setHint("download_url", unity::scopes::Variant(download_url));
        std::string download_sha512 = metadata.scope_data().get_dict()["download_sha512"].get_string();
        activation->setHint("download_sha512", unity::scopes::Variant(download_sha512));
        activation->setHint("action_id", unity::scopes::Variant(action_id));
        qDebug() << "returning ShowPreview";
        activation->setStatus(unity::scopes::ActivationResponse::Status::ShowPreview);
    } else if (action_id == click::Preview::Actions::DOWNLOAD_FAILED) {
        activation->setHint(click::Preview::Actions::DOWNLOAD_FAILED, unity::scopes::Variant(true));
        activation->setStatus(unity::scopes::ActivationResponse::Status::ShowPreview);
    } else if (action_id == click::Preview::Actions::DOWNLOAD_COMPLETED) {
        activation->setHint(click::Preview::Actions::DOWNLOAD_COMPLETED, unity::scopes::Variant(true));
        activation->setHint("installed", unity::scopes::Variant(true));
        activation->setStatus(unity::scopes::ActivationResponse::Status::ShowPreview);
    } else if (action_id == click::Preview::Actions::CANCEL_PURCHASE_INSTALLED) {
        activation->setHint(click::Preview::Actions::CANCEL_PURCHASE_INSTALLED, unity::scopes::Variant(true));
        activation->setStatus(unity::scopes::ActivationResponse::Status::ShowPreview);
    } else if (action_id == click::Preview::Actions::CANCEL_PURCHASE_UNINSTALLED) {
        activation->setHint(click::Preview::Actions::CANCEL_PURCHASE_UNINSTALLED, unity::scopes::Variant(true));
        activation->setStatus(unity::scopes::ActivationResponse::Status::ShowPreview);
    } else if (action_id == click::Preview::Actions::UNINSTALL_CLICK) {
        activation->setHint(click::Preview::Actions::UNINSTALL_CLICK, unity::scopes::Variant(true));
        activation->setStatus(unity::scopes::ActivationResponse::Status::ShowPreview);
    } else if (action_id == click::Preview::Actions::SHOW_UNINSTALLED) {
        activation->setHint(click::Preview::Actions::SHOW_UNINSTALLED, unity::scopes::Variant(true));
        activation->setStatus(unity::scopes::ActivationResponse::Status::ShowPreview);
    } else if (action_id == click::Preview::Actions::SHOW_INSTALLED) {
        activation->setHint(click::Preview::Actions::SHOW_INSTALLED, unity::scopes::Variant(true));
        activation->setStatus(unity::scopes::ActivationResponse::Status::ShowPreview);
    } else if (action_id == click::Preview::Actions::CONFIRM_UNINSTALL) {
        activation->setHint(click::Preview::Actions::CONFIRM_UNINSTALL, unity::scopes::Variant(true));
        activation->setStatus(unity::scopes::ActivationResponse::Status::ShowPreview);
    } else if (action_id == click::Preview::Actions::CONFIRM_CANCEL_PURCHASE_UNINSTALLED) {
        activation->setHint(click::Preview::Actions::CONFIRM_CANCEL_PURCHASE_UNINSTALLED, unity::scopes::Variant(true));
        activation->setStatus(unity::scopes::ActivationResponse::Status::ShowPreview);
    } else if (action_id == click::Preview::Actions::CONFIRM_CANCEL_PURCHASE_INSTALLED) {
        activation->setHint(click::Preview::Actions::CONFIRM_CANCEL_PURCHASE_INSTALLED, unity::scopes::Variant(true));
        activation->setStatus(unity::scopes::ActivationResponse::Status::ShowPreview);
    } else if (action_id == click::Preview::Actions::RATED) {
        scopes::VariantMap rating_info = metadata.scope_data().get_dict();
        // Cast to int because widget gives us double, which is wrong.
        int rating = ((int)rating_info["rating"].get_double());
        std::string review_text = rating_info["review"].get_string();

        // We have to get the values and then set them as hints here, to be
        // able to pass them on to the Preview, which actually makes the
        // call to submit.
        activation->setHint("rating", scopes::Variant(rating));
        activation->setHint("review", scopes::Variant(review_text));
        activation->setHint(click::Preview::Actions::RATED,
                            scopes::Variant(true));
        activation->setHint("widget_id", scopes::Variant(widget_id));
        activation->setStatus(scopes::ActivationResponse::Status::ShowPreview);
    }
    return scopes::ActivationQueryBase::UPtr(activation);
}

#define EXPORT __attribute__ ((visibility ("default")))

extern "C"
{

    EXPORT
    unity::scopes::ScopeBase*
    // cppcheck-suppress unusedFunction
    UNITY_SCOPE_CREATE_FUNCTION()
    {
        // Set up logging
        UbuntuOne::AuthLogger::setupLogging();
#if ENABLE_DEBUG
        UbuntuOne::AuthLogger::setLogLevel(QtDebugMsg);
#else
        const char* u1_debug = getenv("U1_DEBUG");
        if (u1_debug != NULL && strcmp(u1_debug, "") != 0) {
            UbuntuOne::AuthLogger::setLogLevel(QtDebugMsg);
        }
#endif

        return new click::Scope();
    }

    EXPORT
    void
    // cppcheck-suppress unusedFunction
    UNITY_SCOPE_DESTROY_FUNCTION(unity::scopes::ScopeBase* scope_base)
    {
        delete scope_base;
    }

}
