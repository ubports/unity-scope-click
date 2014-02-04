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
#include "fake_json.h"

#include <gtest/gtest.h>
#include <memory>

using namespace ::testing;
using ::testing::_;

const std::string FAKE_SERVER = "http://fake-server/";
const std::string FAKE_PATH = "fake/api/path";
const auto FAKE_QUERY = "FAKE_QUERY";

class MockService : public click::web::Service
{
public:
    using Service::Service;
    MOCK_METHOD2(call, QSharedPointer<click::web::Response>(const std::string& path, const click::web::CallParams& params));
};

class IndexTest : public ::testing::Test {
protected:
    QSharedPointer<MockService> servicePtr;
    QSharedPointer<click::web::Response> responsePtr;
    QSharedPointer<MockNetworkAccessManager> namPtr;
    QSharedPointer<MockNetworkReply> replyPtr;
    std::shared_ptr<click::Index> indexPtr;

    virtual void SetUp() {
        namPtr.reset(new MockNetworkAccessManager());
        replyPtr.reset(new NiceMock<MockNetworkReply>());
        servicePtr.reset(new NiceMock<MockService>(FAKE_SERVER, namPtr));
        responsePtr.reset(new click::web::Response(replyPtr));
        indexPtr.reset(new click::Index(servicePtr));
    }
public:
    MOCK_METHOD1(callback, void(click::PackageList));
};

TEST_F(IndexTest, testSearchCallsWebservice)
{
    EXPECT_CALL(*servicePtr, call(_, _))
            .Times(1)
            .WillOnce(Return(responsePtr));

    indexPtr->search("", [](click::PackageList) {});
}

TEST_F(IndexTest, testSearchSendsQueryAsParam)
{
    click::web::CallParams params;
    params.add(click::QUERY_ARGNAME, FAKE_QUERY);
    EXPECT_CALL(*servicePtr, call(_, params))
            .Times(1)
            .WillOnce(Return(responsePtr));

    indexPtr->search(FAKE_QUERY, [](click::PackageList) {});
}

TEST_F(IndexTest, testSearchSendsRightPath)
{
    EXPECT_CALL(*servicePtr, call(click::SEARCH_PATH, _))
            .Times(1)
            .WillOnce(Return(responsePtr));

    indexPtr->search("", [](click::PackageList) {});
}

TEST_F(IndexTest, testCallbackIsCalled)
{
    QByteArray fake_json("[]");
    ON_CALL(*servicePtr, call(_, _))
            .WillByDefault(Return(responsePtr));
    ON_CALL(*replyPtr, readAll())
            .WillByDefault(Return(fake_json));
    indexPtr->search("", [this](click::PackageList packages){
        callback(packages);
    });
    EXPECT_CALL(*this, callback(_)).Times(1);
    responsePtr->replyFinished();
}

TEST_F(IndexTest, testEmptyJsonIsParsed)
{
    QByteArray fake_json("[]");
    ON_CALL(*servicePtr, call(_, _))
            .WillByDefault(Return(responsePtr));
    ON_CALL(*replyPtr, readAll())
            .WillByDefault(Return(fake_json));
    indexPtr->search("", [this](click::PackageList packages){
        callback(packages);
    });
    click::PackageList empty_package_list;
    EXPECT_CALL(*this, callback(empty_package_list)).Times(1);
    responsePtr->replyFinished();
}

TEST_F(IndexTest, testSingleJsonIsParsed)
{
    QByteArray fake_json(FAKE_JSON_SEARCH_RESULT_ONE.c_str());
    ON_CALL(*servicePtr, call(_, _))
            .WillByDefault(Return(responsePtr));
    ON_CALL(*replyPtr, readAll())
            .WillByDefault(Return(fake_json));
    indexPtr->search("", [this](click::PackageList packages){
        callback(packages);
    });
    click::PackageList single_package_list;
    click::Package fake_package { "org.example.awesomelauncher", "Awesome Launcher", "1.99",
                                  "http://software-center.ubuntu.com/site_media/appmedia/2012/09/SPAZ.png",
                                  "http://search.apps.ubuntu.com/api/v1/package/org.example.awesomelauncher"};
    single_package_list.push_back(fake_package);

    EXPECT_CALL(*this, callback(single_package_list)).Times(1);
    responsePtr->replyFinished();
}

TEST_F(IndexTest, testInvalidJsonIsIgnored)
{
    // TODO
}

TEST_F(IndexTest, testNetworkErrorIgnored)
{
    // TODO
}
