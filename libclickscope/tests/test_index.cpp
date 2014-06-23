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

#include <click/index.h>
#include <click/webclient.h>

#include <tests/mock_network_access_manager.h>
#include <tests/mock_ubuntuone_credentials.h>
#include <tests/mock_webclient.h>

#include "fake_json.h"

#include <gtest/gtest.h>
#include <memory>

using namespace ::testing;

namespace
{

class MockableIndex : public click::Index {
public:
    using click::Index::build_index_query;
    MockableIndex(const QSharedPointer<click::web::Client>& client,
                  const QSharedPointer<click::Configuration> configuration) :
        click::Index(client, configuration)
    {
    }
    MOCK_METHOD0(build_headers, std::map<std::string, std::string>());
    MOCK_METHOD2(build_index_query, std::string(const std::string&, const std::string&));
};

class MockConfiguration : public click::Configuration {
public:
    MOCK_METHOD0(get_architecture, std::string());
    MOCK_METHOD0(get_available_frameworks, std::vector<std::string>());
};


class IndexTest : public ::testing::Test {
protected:
    QSharedPointer<MockClient> clientPtr;
    QSharedPointer<MockNetworkAccessManager> namPtr;
    QSharedPointer<MockConfiguration> configPtr;
    std::shared_ptr<MockableIndex> indexPtr;

    virtual void SetUp() {
        namPtr.reset(new MockNetworkAccessManager());
        clientPtr.reset(new NiceMock<MockClient>(namPtr));
        configPtr.reset(new MockConfiguration());
        indexPtr.reset(new MockableIndex(clientPtr, configPtr));
        // register default value for build_headers() mock
        DefaultValue<std::map<std::string, std::string>>::Set(std::map<std::string, std::string>());
    }
public:
    MOCK_METHOD1(search_callback, void(click::Packages));
    MOCK_METHOD2(details_callback, void(click::PackageDetails, click::Index::Error));
};

class MockPackageManager : public click::PackageManager, public ::testing::Test
{
public:
    MOCK_METHOD2(execute_uninstall_command, void(const std::string&, std::function<void(int, std::string)>));
};

}

TEST_F(IndexTest, testSearchCallsWebservice)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));

    indexPtr->search("", [](click::Packages) {});
}

TEST_F(IndexTest, testSearchSendsBuiltQueryAsParam)
{
    const std::string FAKE_BUILT_QUERY = "FAKE_QUERY,frameworks:fake-14.04,architecture:fake-arch";

    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    click::web::CallParams params;
    params.add(click::QUERY_ARGNAME, FAKE_BUILT_QUERY);
    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, params))
            .Times(1)
            .WillOnce(Return(response));

    EXPECT_CALL(*indexPtr, build_index_query(FAKE_QUERY, ""))
            .Times(1)
            .WillOnce(Return(FAKE_BUILT_QUERY));

    indexPtr->search(FAKE_QUERY, [](click::Packages) {});
}

TEST_F(IndexTest, testSearchSendsRightPath)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(*clientPtr, callImpl(EndsWith(click::SEARCH_PATH),
                                     _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));

    indexPtr->search("", [](click::Packages) {});
}

TEST_F(IndexTest, testSearchCallbackIsCalled)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    QByteArray fake_json("[]");
    EXPECT_CALL(reply.instance, readAll())
            .Times(1)
            .WillOnce(Return(fake_json));
    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));
    EXPECT_CALL(*this, search_callback(_)).Times(1);

    indexPtr->search("", [this](click::Packages packages){
        search_callback(packages);
    });
    response->replyFinished();
}

TEST_F(IndexTest, testSearchEmptyJsonIsParsed)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    QByteArray fake_json("[]");
    EXPECT_CALL(reply.instance, readAll())
            .Times(1)
            .WillOnce(Return(fake_json));
    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));
    click::Packages empty_package_list;
    EXPECT_CALL(*this, search_callback(empty_package_list)).Times(1);

    indexPtr->search("", [this](click::Packages packages){
        search_callback(packages);
    });
    response->replyFinished();
}

TEST_F(IndexTest, testSearchSingleJsonIsParsed)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    QByteArray fake_json(FAKE_JSON_SEARCH_RESULT_ONE.c_str());
    EXPECT_CALL(reply.instance, readAll())
            .Times(1)
            .WillOnce(Return(fake_json));
    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));
    click::Packages single_package_list =
    {
        click::Package
        {
            "org.example.awesomelauncher", "Awesome Launcher", 1.99,
            "http://software-center.ubuntu.com/site_media/appmedia/2012/09/SPAZ.png",
            "http://search.apps.ubuntu.com/api/v1/package/org.example.awesomelauncher"
        }
    };
    EXPECT_CALL(*this, search_callback(single_package_list)).Times(1);

    indexPtr->search("", [this](click::Packages packages){
        search_callback(packages);
    });
    response->replyFinished();
}

TEST_F(IndexTest, testSearchIsCancellable)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));

    auto search_operation = indexPtr->search("", [](click::Packages) {});
    EXPECT_CALL(reply.instance, abort()).Times(1);
    search_operation.cancel();
}

TEST_F(IndexTest, DISABLED_testInvalidJsonIsIgnored)
{
    // TODO, in upcoming branch
}

TEST_F(IndexTest, testSearchNetworkErrorIgnored)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));
    EXPECT_CALL(reply.instance, errorString()).Times(1).WillOnce(Return("fake error"));
    indexPtr->search("", [this](click::Packages packages){
        search_callback(packages);
    });

    click::Packages empty_package_list;
    EXPECT_CALL(*this, search_callback(empty_package_list)).Times(1);

    emit reply.instance.error(QNetworkReply::UnknownNetworkError);
}

TEST_F(IndexTest, testGetDetailsCallsWebservice)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));

    indexPtr->get_details("", [](click::PackageDetails, click::Index::Error) {});
}

TEST_F(IndexTest, testGetDetailsSendsPackagename)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(*clientPtr, callImpl(EndsWith(FAKE_PACKAGENAME),
                                     _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));

    indexPtr->get_details(FAKE_PACKAGENAME, [](click::PackageDetails, click::Index::Error) {});
}

TEST_F(IndexTest, testGetDetailsSendsRightPath)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(*clientPtr, callImpl(StartsWith(click::SEARCH_BASE_URL +
                                                click::DETAILS_PATH),
                                     _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));

    indexPtr->get_details(FAKE_PACKAGENAME, [](click::PackageDetails, click::Index::Error) {});
}

TEST_F(IndexTest, testGetDetailsCallbackIsCalled)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    QByteArray fake_json(FAKE_JSON_PACKAGE_DETAILS.c_str());
    EXPECT_CALL(reply.instance, readAll())
            .Times(1)
            .WillOnce(Return(fake_json));
    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));
    indexPtr->get_details("", [this](click::PackageDetails details, click::Index::Error error){
        details_callback(details, error);
    });
    EXPECT_CALL(*this, details_callback(_, _)).Times(1);
    response->replyFinished();
}

TEST_F(IndexTest, testGetDetailsJsonIsParsed)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    QByteArray fake_json(FAKE_JSON_PACKAGE_DETAILS.c_str());
    EXPECT_CALL(reply.instance, readAll())
            .Times(1)
            .WillOnce(Return(fake_json));
    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));
    indexPtr->get_details("", [this](click::PackageDetails details, click::Index::Error error){
        details_callback(details, error);
    });
    click::PackageDetails fake_details {
        {
            "ar.com.beuno.wheather-touch",
            "\u1F4A9 Weather",
            1.99,
            "http://developer.staging.ubuntu.com/site_media/appmedia/2013/07/weather-icone-6797-64.png",
            "https://public.apps.staging.ubuntu.com/download/ar.com.beuno/wheather-touch/ar.com.beuno.wheather-touch-0.2",
            "0.2",
        },
        "\u1F4A9 Weather\nA weather application.",
        "https://public.apps.staging.ubuntu.com/download/ar.com.beuno/wheather-touch/ar.com.beuno.wheather-touch-0.2",
        3.5,
        "these, are, key, words",
        "tos",
        "Proprietary",
        "Beuno",
        "sshot0",
        {"sshot1", "sshot2"},
        177582,
        "0.2",
        "None"
    };
    EXPECT_CALL(*this, details_callback(fake_details, _)).Times(1);
    response->replyFinished();
}

TEST_F(IndexTest, testGetDetailsJsonUtf8)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    QByteArray appname_utf8("\xe5\xb0\x8f\xe6\xb5\xb7");
    QByteArray appname_json("\\u5c0f\\u6d77");

    qDebug() << "testGetDetailsJsonUtf8, title:" << appname_utf8.toPercentEncoding(" ");
    QByteArray fake_json = QByteArray(FAKE_JSON_PACKAGE_DETAILS.c_str()).replace("Weather", appname_json);

    EXPECT_CALL(reply.instance, readAll())
            .Times(1)
            .WillOnce(Return(fake_json));
    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));
    indexPtr->get_details("", [this, appname_utf8](click::PackageDetails details, click::Index::Error error){
        details_callback(details, error);
    });

    click::PackageDetails fake_details {
        {
            "ar.com.beuno.wheather-touch",
            std::string("\u1F4A9 ") + appname_utf8.constData(),
            1.99,
            "http://developer.staging.ubuntu.com/site_media/appmedia/2013/07/weather-icone-6797-64.png",
            "https://public.apps.staging.ubuntu.com/download/ar.com.beuno/wheather-touch/ar.com.beuno.wheather-touch-0.2",
            "v0.1",
        },
        (std::string("\u1F4A9 ") + std::string(appname_utf8.constData()) + "\nA weather application.").c_str(),
        "https://public.apps.staging.ubuntu.com/download/ar.com.beuno/wheather-touch/ar.com.beuno.wheather-touch-0.2",
        3.5,
        "these, are, key, words",
        "tos",
        "Proprietary",
        "Beuno",
        "sshot0",
        {"sshot1", "sshot2"},
        177582,
        "0.2",
        "None"
    };

    EXPECT_CALL(*this, details_callback(fake_details, _)).Times(1);
    response->replyFinished();
}


TEST_F(IndexTest, testGetDetailsNetworkErrorReported)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));
    EXPECT_CALL(reply.instance, errorString()).Times(1).WillOnce(Return("fake error"));
    indexPtr->get_details("", [this](click::PackageDetails details, click::Index::Error error){
        details_callback(details, error);
    });
    EXPECT_CALL(*this, details_callback(_, click::Index::Error::NetworkError)).Times(1);

    emit reply.instance.error(QNetworkReply::UnknownNetworkError);
}

TEST_F(IndexTest, testGetDetailsIsCancellable)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));

    auto get_details_operation = indexPtr->get_details("", [](click::PackageDetails, click::Index::Error) {});
    EXPECT_CALL(reply.instance, abort()).Times(1);
    get_details_operation.cancel();
}

TEST_F(IndexTest, testGetBaseUrl)
{
    const char *value = getenv(click::SEARCH_BASE_URL_ENVVAR.c_str());
    if (value != NULL) {
        ASSERT_TRUE(unsetenv(click::SEARCH_BASE_URL_ENVVAR.c_str()) == 0);
    }
    ASSERT_TRUE(click::Index::get_base_url() == click::SEARCH_BASE_URL);
    
}

TEST_F(IndexTest, testGetBaseUrlFromEnv)
{
    ASSERT_TRUE(setenv(click::SEARCH_BASE_URL_ENVVAR.c_str(),
                       FAKE_SERVER.c_str(), 1) == 0);
    ASSERT_TRUE(click::Index::get_base_url() == FAKE_SERVER);
    ASSERT_TRUE(unsetenv(click::SEARCH_BASE_URL_ENVVAR.c_str()) == 0);
}

TEST_F(MockPackageManager, testUninstallCommandCorrect)
{
    click::Package package = {
        "org.example.testapp", "Test App", 0.00,
        "/tmp/foo.png",
        "uri", "0.1.5"
    };
    std::string expected = "pkcon -p remove org.example.testapp;0.1.5;all;local:click";
    EXPECT_CALL(*this, execute_uninstall_command(expected, _)).Times(1);
    uninstall(package, [](int, std::string) {});
}

class ExhibitionistIndex : public click::Index {
public:
    using click::Index::build_index_query;
    using click::Index::build_headers;
    ExhibitionistIndex(const QSharedPointer<click::web::Client>& client,
                       const QSharedPointer<click::Configuration> configuration) :
        click::Index(client, configuration)
    {
    }
};

class QueryStringTest : public ::testing::Test {
protected:
    const std::string fake_arch{"fake_arch"};
    const std::string fake_fwk_1{"fake_fwk_1"};
    const std::string fake_fwk_2{"fake_fwk_2"};
    std::vector<std::string> fake_frameworks{fake_fwk_1, fake_fwk_2};

    QSharedPointer<MockClient> clientPtr;
    QSharedPointer<MockNetworkAccessManager> namPtr;
    QSharedPointer<MockConfiguration> configPtr;
    std::shared_ptr<ExhibitionistIndex> indexPtr;

    virtual void SetUp() {
        namPtr.reset(new MockNetworkAccessManager());
        clientPtr.reset(new NiceMock<MockClient>(namPtr));
        configPtr.reset(new MockConfiguration());
        indexPtr.reset(new ExhibitionistIndex(clientPtr, configPtr));
    }
};


TEST_F(QueryStringTest, testBuildHeadersAddsArchitecture)
{
    EXPECT_CALL(*configPtr, get_architecture()).Times(1).WillOnce(Return(fake_arch));
    EXPECT_CALL(*configPtr, get_available_frameworks()).Times(1).WillOnce(Return(fake_frameworks));
    auto hdrs = indexPtr->build_headers();
    EXPECT_EQ(fake_arch, hdrs["X-Ubuntu-Architecture"]);
}

TEST_F(QueryStringTest, testBuildHeadersAddsFramework)
{
    EXPECT_CALL(*configPtr, get_architecture()).Times(1).WillOnce(Return(fake_arch));
    EXPECT_CALL(*configPtr, get_available_frameworks()).Times(1).WillOnce(Return(fake_frameworks));
    auto hdrs = indexPtr->build_headers();
    EXPECT_NE(std::string::npos, hdrs["X-Ubuntu-Frameworks"].find(fake_fwk_1));
    EXPECT_NE(std::string::npos, hdrs["X-Ubuntu-Frameworks"].find(fake_fwk_2));
}
