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
#include <click/configuration.h>
#include <click/reviews.h>
#include <click/webclient.h>
#include <click/index.h>

#include <tests/mock_network_access_manager.h>
#include <tests/mock_ubuntuone_credentials.h>
#include <tests/mock_webclient.h>

#include <gtest/gtest.h>
#include "fake_json.h"
#include <json/reader.h>
#include <json/value.h>
#include <click/highlights.h>
#include <click/configuration.h>
#include <click/departments.h>

#include <click/webclient.h>

using namespace ::testing;

namespace
{

class BootstrapTest: public ::testing::Test {
protected:
    QSharedPointer<MockClient> clientPtr;
    QSharedPointer<MockNetworkAccessManager> namPtr;
    std::shared_ptr<click::Index> indexPtr;

    virtual void SetUp() {
        namPtr.reset(new MockNetworkAccessManager());
        clientPtr.reset(new NiceMock<MockClient>(namPtr));
        indexPtr.reset(new click::Index(clientPtr));
    }

public:
    MOCK_METHOD4(bootstrap_callback, void(const click::DepartmentList&, const click::HighlightList&, click::Index::Error, int));
};
}

TEST_F(BootstrapTest, testBootstrapCallsWebservice)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(*clientPtr, callImpl(EndsWith(click::BOOTSTRAP_PATH), "GET", _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));
    indexPtr->bootstrap([](const click::DepartmentList&, const click::HighlightList&, click::Index::Error, int) {});
}

TEST_F(BootstrapTest, testBootstrapJsonIsParsed)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    QByteArray fake_json(FAKE_JSON_BOOTSTRAP.c_str());
    EXPECT_CALL(reply.instance, readAll())
            .Times(1)
            .WillOnce(Return(fake_json));

    EXPECT_CALL(*clientPtr, callImpl(EndsWith(click::BOOTSTRAP_PATH), "GET", _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));

    EXPECT_CALL(*this, bootstrap_callback(_, _, _, _)).Times(1);
    indexPtr->bootstrap([this](const click::DepartmentList& depts, const click::HighlightList& highlights, click::Index::Error error, int error_code) {
        bootstrap_callback(depts, highlights, error, error_code);
        {
            EXPECT_EQ(3u, highlights.size());
            auto it = highlights.begin();
            EXPECT_EQ("top-apps", it->slug());
            EXPECT_EQ("Top Apps", it->name());
            EXPECT_EQ(2u, it->packages().size());
            ++it;
            EXPECT_EQ("most-purchased", it->slug());
            EXPECT_EQ("Most Purchased", it->name());
            EXPECT_EQ(2u, it->packages().size());
            ++it;
            EXPECT_EQ("new-releases", it->slug());
            EXPECT_EQ("New Releases", it->name());
            EXPECT_EQ(2u, it->packages().size());
        }
        {
            EXPECT_EQ(1u, depts.size());
            auto it = depts.begin();
            EXPECT_EQ("Fake Subdepartment", (*it)->name());
            EXPECT_FALSE((*it)->has_children_flag());
            EXPECT_EQ("https://search.apps.staging.ubuntu.com/api/v1/departments/fake-subdepartment", (*it)->href());
        }
    });
    response->replyFinished();
}

TEST_F(BootstrapTest, testParsing)
{
    Json::Reader reader;
    Json::Value root;

    EXPECT_TRUE(reader.parse(FAKE_JSON_BOOTSTRAP, root));

    {
        auto highlights = click::Highlight::from_json_root_node(root);
        EXPECT_EQ(3u, highlights.size());
        auto it = highlights.begin();
        EXPECT_EQ("Top Apps", it->name());
        EXPECT_EQ(2u, it->packages().size());
        ++it;
        EXPECT_EQ("Most Purchased", it->name());
        EXPECT_EQ(2u, it->packages().size());
        ++it;
        EXPECT_EQ("New Releases", it->name());
        EXPECT_EQ(2u, it->packages().size());
    }
    {
        auto depts = click::Department::from_json_root_node(root);
        EXPECT_EQ(1u, depts.size());
        auto it = depts.begin();
        EXPECT_EQ("Fake Subdepartment", (*it)->name());
        EXPECT_FALSE((*it)->has_children_flag());
        EXPECT_EQ("https://search.apps.staging.ubuntu.com/api/v1/departments/fake-subdepartment", (*it)->href());
    }
}

TEST_F(BootstrapTest, testParsingErrors)
{
    Json::Reader reader;
    Json::Value root;

    EXPECT_TRUE(reader.parse(FAKE_JSON_BROKEN_BOOTSTRAP, root));

    {
        auto highlights = click::Highlight::from_json_root_node(root);
        EXPECT_EQ(1u, highlights.size());
        auto it = highlights.begin();
        EXPECT_EQ("Top Apps", it->name());
        EXPECT_EQ(1u, it->packages().size());
    }
    {
        auto depts = click::Department::from_json_root_node(root);
        EXPECT_EQ(0u, depts.size());
    }
}

TEST_F(BootstrapTest, testDepartmentAllApps)
{
    Json::Reader reader;
    Json::Value root;

    EXPECT_TRUE(reader.parse(FAKE_JSON_DEPARTMENT_WITH_APPS, root));

    {
        auto highlights = click::Highlight::from_json_root_node(root);
        EXPECT_EQ(5u, highlights.size());
        auto it = highlights.begin();
        EXPECT_EQ("Top Apps", it->name());
        EXPECT_EQ(2u, it->packages().size());
        EXPECT_EQ(false, it->contains_scopes());
        ++it;
        EXPECT_EQ("Most Purchased", it->name());
        EXPECT_EQ(2u, it->packages().size());
        EXPECT_EQ(false, it->contains_scopes());
        ++it;
        EXPECT_EQ("New Releases", it->name());
        EXPECT_EQ(2u, it->packages().size());
        EXPECT_EQ(false, it->contains_scopes());
        ++it;
        EXPECT_EQ("Apps", it->name());
        EXPECT_EQ(2u, it->packages().size());
        EXPECT_EQ(false, it->contains_scopes());
        ++it;
        EXPECT_EQ("Scopes", it->name());
        EXPECT_EQ(2u, it->packages().size());
        EXPECT_EQ(true, it->contains_scopes());
    }

}
