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
#include "webclient.h"
#include "network_access_manager.h"
#include "key_file_locator.h"
#include "interface.h"

#include <QSharedPointer>
#include <QDesktopServices>
#include <QUrl>

namespace
{
click::Interface& clickInterfaceInstance()
{
    static QSharedPointer<click::KeyFileLocator> keyFileLocator(new click::KeyFileLocator());
    static click::Interface iface(keyFileLocator);

    return iface;
}
}

class ScopeActivation : public unity::scopes::ActivationBase
{
    unity::scopes::ActivationResponse activate() override
    {
        auto response = unity::scopes::ActivationResponse(status_);
        response.setHints(hints_);
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
    QSharedPointer<click::web::Service> servicePtr(
                new click::web::Service(click::SEARCH_BASE_URL, nam));
    index = QSharedPointer<click::Index>(new click::Index(servicePtr));
    downloader = QSharedPointer<click::Downloader>(new click::Downloader(nam));
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

scopes::QueryBase::UPtr click::Scope::create_query(unity::scopes::Query const& q, scopes::SearchMetadata const&)
{
    return scopes::QueryBase::UPtr(new click::Query(q.query_string()));
}


unity::scopes::QueryBase::UPtr click::Scope::preview(const unity::scopes::Result& result,
        const unity::scopes::ActionMetadata& metadata) {
    qDebug() << "Preview called.";
    std::string action_id = "";
    std::string download_url = "";

    if (metadata.scope_data().which() != scopes::Variant::Type::Null) {
        auto metadict = metadata.scope_data().get_dict();

        if(metadict.count(click::Preview::Actions::DOWNLOAD_FAILED) != 0) {
            return scopes::QueryBase::UPtr{new ErrorPreview(std::string("Download or install failed. Please try again."),
                                                            index, result)};
        } else if (metadict.count(click::Preview::Actions::DOWNLOAD_COMPLETED) != 0  ||
                   metadict.count(click::Preview::Actions::CLOSE_PREVIEW) != 0) {
            Preview* prev = new Preview(result.uri(), index, result);
            prev->setPreview(click::Preview::Type::INSTALLED);
            return scopes::QueryBase::UPtr{prev};
        } else if (metadict.count("action_id") != 0  &&
            metadict.count("download_url") != 0) {
            action_id = metadict["action_id"].get_string();
            download_url = metadict["download_url"].get_string();
            if (action_id == click::Preview::Actions::INSTALL_CLICK) {
                return scopes::QueryBase::UPtr{new InstallPreview(download_url, index, result, nam)};
            }
        } else if (metadict.count(click::Preview::Actions::UNINSTALL_CLICK) != 0) {
            Preview* prev = new Preview(result.uri(), index, result);
            prev->setPreview(click::Preview::Type::CONFIRM_UNINSTALL);
            return scopes::QueryBase::UPtr{prev};
        } else if (metadict.count(click::Preview::Actions::CONFIRM_UNINSTALL) != 0) {
            Preview* prev = new Preview(result.uri(), index, result);
            prev->setPreview(click::Preview::Type::UNINSTALL);
            return scopes::QueryBase::UPtr{prev};
        }
    }
    scopes::QueryBase::UPtr previewResult(new Preview(result.uri(), index, result));
    return previewResult;
}

unity::scopes::ActivationBase::UPtr click::Scope::perform_action(unity::scopes::Result const& result, unity::scopes::ActionMetadata const& metadata, std::string const& /* widget_id */, std::string const& action_id)
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
                             QString app_desktop("application:///");
                             app_desktop.append(QString::fromStdString(val));
                             QDesktopServices::openUrl(QUrl(app_desktop));
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
    }
    return scopes::ActivationBase::UPtr(activation);
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
