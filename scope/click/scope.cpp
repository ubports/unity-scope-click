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

#include "qtbridge.h"
#include "scope.h"
#include "query.h"
#include "preview.h"
#include "network_access_manager.h"
#include "key_file_locator.h"
#include "interface.h"

#include <QSharedPointer>
#include <url-dispatcher.h>

namespace
{
click::Interface& clickInterfaceInstance()
{
    static QSharedPointer<click::KeyFileLocator> keyFileLocator(new click::KeyFileLocator());
    static click::Interface iface(keyFileLocator);

    return iface;
}
}

class ScopeActivation : public unity::scopes::ActivationQueryBase
{
    unity::scopes::ActivationResponse activate() override
    {
        auto response = unity::scopes::ActivationResponse(status_);
        response.set_scope_data(unity::scopes::Variant(hints_));
        return response;
    }

public:
    void setStatus(unity::scopes::ActivationResponse::Status status) { status_ = status; }
    void setHint(std::string key, unity::scopes::Variant value) { hints_[key] = value; }
private:
    unity::scopes::ActivationResponse::Status status_ = unity::scopes::ActivationResponse::Status::ShowPreview;
    unity::scopes::VariantMap hints_;
};

click::Scope::Scope()
{
    nam = QSharedPointer<click::network::AccessManager>(new click::network::AccessManager());
    sso = QSharedPointer<click::CredentialsService>(new click::CredentialsService());
    client = QSharedPointer<click::web::Client>(new click::web::Client(nam, sso));
}

click::Scope::~Scope()
{
}

int click::Scope::start(std::string const&, scopes::RegistryProxy const&)
{
    return VERSION;
}

void click::Scope::run()
{
    static const int zero = 0;
    auto emptyCb = [this]()
    {

    };

    qt::core::world::build_and_run(zero, nullptr, emptyCb);
}

void click::Scope::stop()
{
    qt::core::world::destroy();
}

scopes::SearchQueryBase::UPtr click::Scope::search(unity::scopes::CannedQuery const& q, scopes::SearchMetadata const& metadata)
{
    return scopes::SearchQueryBase::UPtr(new click::Query(q.query_string(), metadata));
}


unity::scopes::PreviewQueryBase::UPtr click::Scope::preview(const unity::scopes::Result& result,
        const unity::scopes::ActionMetadata& metadata) {
    qDebug() << "Scope::preview() called.";
    std::string action_id = "";
    std::string download_url = "";

    if (metadata.scope_data().which() != scopes::Variant::Type::Null) {
        auto metadict = metadata.scope_data().get_dict();

        if (metadict.count(click::Preview::Actions::DOWNLOAD_FAILED) != 0) {
            return scopes::PreviewQueryBase::UPtr{new DownloadErrorPreview(result)};
        } else if (metadict.count(click::Preview::Actions::DOWNLOAD_COMPLETED) != 0  ||
                   metadict.count(click::Preview::Actions::CLOSE_PREVIEW) != 0) {
            qDebug() << "in Scope::preview(), metadata has download_completed=" 
                     << metadict.count(click::Preview::Actions::DOWNLOAD_COMPLETED)
                     << " and close_preview=" 
                     << metadict.count(click::Preview::Actions::CLOSE_PREVIEW);

            return scopes::PreviewQueryBase::UPtr{new InstalledPreview(result, client)};
        } else if (metadict.count("action_id") != 0  &&
            metadict.count("download_url") != 0) {
            action_id = metadict["action_id"].get_string();
            download_url = metadict["download_url"].get_string();
            if (action_id == click::Preview::Actions::INSTALL_CLICK) {
                return scopes::PreviewQueryBase::UPtr{new InstallingPreview(download_url, result, client, nam)};
            } else {
                qWarning() << "unexpected action id " << QString::fromStdString(action_id)
                           << " given with download_url" << QString::fromStdString(download_url);
                return scopes::PreviewQueryBase::UPtr{new UninstalledPreview(result, client)};
            }
        } else if (metadict.count(click::Preview::Actions::UNINSTALL_CLICK) != 0) {
            return scopes::PreviewQueryBase::UPtr{ new UninstallConfirmationPreview(result)};
        } else if (metadict.count(click::Preview::Actions::CONFIRM_UNINSTALL) != 0) {
            return scopes::PreviewQueryBase::UPtr{new UninstallingPreview(result, client)};
        } else {
            qWarning() << "preview() called with unexpected metadata. returning uninstalled preview";
            return scopes::PreviewQueryBase::UPtr{new UninstalledPreview(result, client)};            
        }
    } else {
        // metadata.scope_data() is Null, so we return an appropriate "default" preview:
        if (result["installed"].get_bool() == true) {
            return scopes::PreviewQueryBase::UPtr{new InstalledPreview(result, client)};
        } else {
            return scopes::PreviewQueryBase::UPtr{new UninstalledPreview(result, client)};
        }
    }
}

unity::scopes::ActivationQueryBase::UPtr click::Scope::perform_action(unity::scopes::Result const& result, unity::scopes::ActionMetadata const& metadata, std::string const& /* widget_id */, std::string const& action_id)
{
    auto activation = new ScopeActivation();
    qDebug() << "perform_action called with action_id" << QString().fromStdString(action_id);

    if (action_id == click::Preview::Actions::OPEN_CLICK) {
        QString app_url = QString::fromStdString(result.uri());
        if (!app_url.startsWith("application:///")) {
            qt::core::world::enter_with_task([this, result] (qt::core::world::Environment& /*env*/)
            {
                clickInterfaceInstance().get_dotdesktop_filename(result["name"].get_string(),
                     [] (std::string val, click::ManifestError error){
                         if (error == click::ManifestError::NoError) {
                             std::string uri = "application:///" + val;
                             url_dispatch_send(uri.c_str() , NULL, NULL);
                         }
                     }
                );
            });
            activation->setStatus(unity::scopes::ActivationResponse::Status::HideDash);
        } else {
            activation->setStatus(unity::scopes::ActivationResponse::Status::NotHandled);
        }
    } else if (action_id == click::Preview::Actions::INSTALL_CLICK) {
        std::string download_url = metadata.scope_data().get_dict()["download_url"].get_string();
        qDebug() << "the download url is: " << QString::fromStdString(download_url);
        activation->setHint("download_url", unity::scopes::Variant(download_url));
        activation->setHint("action_id", unity::scopes::Variant(action_id));
        qDebug() << "returning ShowPreview";
        activation->setStatus(unity::scopes::ActivationResponse::Status::ShowPreview);

    } else if (action_id == click::Preview::Actions::DOWNLOAD_FAILED) {
        activation->setHint(click::Preview::Actions::DOWNLOAD_FAILED, unity::scopes::Variant(true));
        activation->setStatus(unity::scopes::ActivationResponse::Status::ShowPreview);
    } else if (action_id == click::Preview::Actions::DOWNLOAD_COMPLETED) {
        activation->setHint(click::Preview::Actions::DOWNLOAD_COMPLETED, unity::scopes::Variant(true));
        activation->setStatus(unity::scopes::ActivationResponse::Status::ShowPreview);
    } else if (action_id == click::Preview::Actions::UNINSTALL_CLICK) {
        activation->setHint(click::Preview::Actions::UNINSTALL_CLICK, unity::scopes::Variant(true));
        activation->setStatus(unity::scopes::ActivationResponse::Status::ShowPreview);
    } else if (action_id == click::Preview::Actions::CLOSE_PREVIEW) {
        activation->setHint(click::Preview::Actions::CLOSE_PREVIEW, unity::scopes::Variant(true));
        activation->setStatus(unity::scopes::ActivationResponse::Status::ShowPreview);
    } else if (action_id == click::Preview::Actions::CONFIRM_UNINSTALL) {
        activation->setHint(click::Preview::Actions::CONFIRM_UNINSTALL, unity::scopes::Variant(true));
        activation->setStatus(unity::scopes::ActivationResponse::Status::ShowPreview);
    } else if (action_id == click::Preview::Actions::OPEN_ACCOUNTS) {
        std::string uri = "settings:///system/online-accounts";
        url_dispatch_send(uri.c_str() , NULL, NULL);
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
