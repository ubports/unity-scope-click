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

#include "ubuntuone_credentials.h"

#include <future>
#include <QCoreApplication>

namespace u1 = UbuntuOne;

click::CredentialsService::CredentialsService()
    : ssoService(new u1::SSOService())
{
    // Forward signals directly:
    connect(ssoService.data(), &u1::SSOService::credentialsFound,
            this, &click::CredentialsService::credentialsFound);
    connect(ssoService.data(), &u1::SSOService::credentialsNotFound,
            this, &click::CredentialsService::credentialsNotFound);
    connect(ssoService.data(), &u1::SSOService::credentialsDeleted,
            this, &click::CredentialsService::credentialsDeleted);
}

click::CredentialsService::~CredentialsService()
{
}

UbuntuOne::Token click::CredentialsService::getToken()
{
    if (!_token.isValid()) {
        std::promise<UbuntuOne::Token> promise;
        auto future = promise.get_future();

        auto success = QObject::connect(ssoService.data(),
                                        &u1::SSOService::credentialsFound,
                                        [this, &promise](const u1::Token& token) {
                                            emit credentialsFound(_token);
                                            try { promise.set_value(token); }
                                            catch (const std::future_error&) {} // Ignore promise_already_satisfied
                                        });
        auto notfound = QObject::connect(ssoService.data(),
                                         &u1::SSOService::credentialsNotFound,
                                         [this, &promise]() {
                                             qWarning() << "No Ubuntu One token found.";
                                             emit credentialsNotFound();
                                             try { promise.set_value(u1::Token()); }
                                             catch (const std::future_error&) {} // Ignore promise_already_satisfied
                                         });

        getCredentials();

        std::future_status status = future.wait_for(std::chrono::milliseconds(0));
        while (status != std::future_status::ready) {
            QCoreApplication::processEvents();
            qDebug() << "Processed some events, waiting to process again.";
            status = future.wait_for(std::chrono::milliseconds(100));
        }

        _token = future.get();
        QObject::disconnect(success);
        QObject::disconnect(notfound);
    }

    return _token;
}

void click::CredentialsService::getCredentials()
{
    ssoService->getCredentials();
}

void click::CredentialsService::invalidateCredentials()
{
    ssoService->invalidateCredentials();
}

