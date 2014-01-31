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

#include "click/index.h"
#include "click/webclient.h"

#include "mock_network_access_manager.h"

#include <gtest/gtest.h>

class MockService : public click::web::Service
{
public:
    MockService()
    {
    }
    MOCK_METHOD2(call, QSharedPointer<click::web::Response>(const QString& path, const click::web::CallParams& params));
};

class MockResponse : public click::web::Response
{
public:
    MockResponse()
    {
    }
    MOCK_METHOD0(replyFinished, void());
};

TEST(Index, testSearchCallsWebservice)
{
    using namespace ::testing;
    using ::testing::_;

    MockService service;
    QSharedPointer<click::web::Service> servicePtr(
                &service,
                [](click::web::Service*) {});

    click::Index index(servicePtr);

    MockResponse response;
    QSharedPointer<click::web::Response> responsePtr(
                &response,
                [](click::web::Response*) {});

    ON_CALL(service, call(_, _)).WillByDefault(Return(response));
    EXPECT_CALL(service, call(_, _));//.Times(1).WillOnce(Return(response));

    index.search("", [](std::list<click::Package>) {});
}

