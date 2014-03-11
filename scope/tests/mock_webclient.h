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

#ifndef MOCK_WEBCLIENT_H
#define MOCK_WEBCLIENT_H

#include <click/webclient.h>
#include <click/ubuntuone_credentials.h>

#include <gtest/gtest.h>

using namespace ::testing;

namespace
{

const std::string FAKE_SERVER = "http://fake-server/";
const std::string FAKE_PATH = "fake/api/path";
const std::string FAKE_QUERY = "FAKE_QUERY";
const std::string FAKE_PACKAGENAME = "com.example.fakepackage";

template<typename Interface, typename Mock>
struct LifetimeHelper
{
    LifetimeHelper() : instance()
    {
    }

    template<typename... Args>
    LifetimeHelper(Args&&... args) : instance(std::forward<Args...>(args...))
    {
    }

    QSharedPointer<Interface> asSharedPtr()
    {
        return QSharedPointer<Interface>(&instance, [](Interface*){});
    }

    Mock instance;
};

QSharedPointer<click::web::Response> responseForReply(const QSharedPointer<click::network::Reply>& reply)
{
    auto response = QSharedPointer<click::web::Response>(new click::web::Response(QSharedPointer<QBuffer>(new QBuffer())));
    response->setReply(reply);
    return response;
}

class MockClient : public click::web::Client
{
public:
    MockClient(const QSharedPointer<click::network::AccessManager>& networkAccessManager,
                const QSharedPointer<click::CredentialsService>& sso)
        : Client(networkAccessManager, sso)
    {
    }

    // Mocking default arguments: https://groups.google.com/forum/#!topic/googlemock/XrabW20vV7o
    MOCK_METHOD6(callImpl, QSharedPointer<click::web::Response>(
        const std::string& iri,
        const std::string& method,
        bool sign,
        const std::map<std::string, std::string>& headers,
        const std::string& data,
        const click::web::CallParams& params));
    QSharedPointer<click::web::Response> call(
        const std::string& iri,
        const click::web::CallParams& params=click::web::CallParams()) {
        return callImpl(iri, "GET", false,
                        std::map<std::string, std::string>(), "", params);
    }
    QSharedPointer<click::web::Response> call(
        const std::string& iri,
        const std::string& method,
        bool sign = false,
        const std::map<std::string, std::string>& headers = std::map<std::string, std::string>(),
        const std::string& data = "",
        const click::web::CallParams& params=click::web::CallParams()) {
        return callImpl(iri, method, sign, headers, data, params);
    }
};

}

#endif // MOCK_WEBCLIENT_H
