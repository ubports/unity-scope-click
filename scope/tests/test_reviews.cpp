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

#include <click/reviews.h>
#include <click/webclient.h>

#include <tests/mock_network_access_manager.h>

#include "fake_json.h"
#include "mock_ubuntuone_credentials.h"
#include "mock_webclient.h"

#include <gtest/gtest.h>
#include <memory>
#include <stdlib.h>

using namespace ::testing;

namespace
{

class ReviewsTest : public ::testing::Test {
protected:
    QSharedPointer<MockClient> clientPtr;
    QSharedPointer<MockNetworkAccessManager> namPtr;
    std::shared_ptr<click::Reviews> reviewsPtr;

    virtual void SetUp() {
        namPtr.reset(new MockNetworkAccessManager());
        clientPtr.reset(new NiceMock<MockClient>(namPtr));
        reviewsPtr.reset(new click::Reviews(clientPtr));
    }

public:
    MOCK_METHOD2(reviews_callback, void(click::ReviewList, click::Reviews::Error));
};
}

TEST_F(ReviewsTest, testFetchReviewsCallsWebservice)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));

    reviewsPtr->fetch_reviews("", [](click::ReviewList,
                                     click::Reviews::Error) {});
}

TEST_F(ReviewsTest, testFetchReviewsSendsQueryAsParam)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    click::web::CallParams params;
    params.add(click::REVIEWS_QUERY_ARGNAME, FAKE_PACKAGENAME);
    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, params))
            .Times(1)
            .WillOnce(Return(response));

    reviewsPtr->fetch_reviews(FAKE_PACKAGENAME, [](click::ReviewList,
                                                   click::Reviews::Error) {});
}

TEST_F(ReviewsTest, testFetchReviewsSendsCorrectPath)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(*clientPtr, callImpl(EndsWith(click::REVIEWS_API_PATH),
                                     _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));

    reviewsPtr->fetch_reviews(FAKE_PACKAGENAME, [](click::ReviewList,
                                                   click::Reviews::Error) {});
}

TEST_F(ReviewsTest, testFetchReviewsCallbackCalled)
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
    EXPECT_CALL(*this, reviews_callback(_, _)).Times(1);

    reviewsPtr->fetch_reviews("", [this](click::ReviewList reviews,
                                         click::Reviews::Error error){
                                  reviews_callback(reviews, error);
                              });
    response->replyFinished();
}

TEST_F(ReviewsTest, testFetchReviewsEmptyJsonIsParsed)
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
    click::ReviewList empty_reviews_list;
    EXPECT_CALL(*this, reviews_callback(empty_reviews_list, _)).Times(1);

    reviewsPtr->fetch_reviews("", [this](click::ReviewList reviews,
                                         click::Reviews::Error error){
                                  reviews_callback(reviews, error);
                              });
    response->replyFinished();
}

TEST_F(ReviewsTest, testFetchReviewsSingleJsonIsParsed)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    QByteArray fake_json(FAKE_JSON_REVIEWS_RESULT_ONE.c_str());
    EXPECT_CALL(reply.instance, readAll())
            .Times(1)
            .WillOnce(Return(fake_json));
    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));
    click::ReviewList single_review_list =
    {
        click::Review
        {
            1, 4, 0, 0, false,
            "2014-01-28T09:09:47.218Z", "null",
            FAKE_PACKAGENAME, "0.2",
            "en",
            "Review Summary", "It is ok.",
            "Reviewer", "reviewer"
        }
    };
    EXPECT_CALL(*this, reviews_callback(single_review_list, _)).Times(1);

    reviewsPtr->fetch_reviews("", [this](click::ReviewList reviews,
                                         click::Reviews::Error error){
                                  reviews_callback(reviews, error);
                              });
    response->replyFinished();
}

TEST_F(ReviewsTest, testFetchReviewsNetworkErrorReported)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));
    EXPECT_CALL(reply.instance, errorString())
        .Times(1)
        .WillOnce(Return("fake error"));
    reviewsPtr->fetch_reviews("", [this](click::ReviewList reviews,
                                         click::Reviews::Error error){
                                  reviews_callback(reviews, error);
                              });
    click::ReviewList empty_reviews_list;
    EXPECT_CALL(*this, reviews_callback(empty_reviews_list,
                                        click::Reviews::Error::NetworkError))
        .Times(1);

    emit reply.instance.error(QNetworkReply::UnknownNetworkError);
}

TEST_F(ReviewsTest, testFetchReviewsIsCancellable)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    EXPECT_CALL(*clientPtr, callImpl(_, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(response));

    auto fetch_reviews_op = reviewsPtr->fetch_reviews("", [](click::ReviewList,
                                                             click::Reviews::Error) {});
    EXPECT_CALL(reply.instance, abort()).Times(1);
    fetch_reviews_op.cancel();
}

TEST_F(ReviewsTest, testSubmitReviewIsCancellable)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    click::Review review;
    review.rating = 3;
    review.review_text = "A review";
    review.package_name = "com.example.test";
    review.package_version = "0.1";

    EXPECT_CALL(*clientPtr, callImpl(_, "POST", true, _, _, _))
            .Times(1)
            .WillOnce(Return(response));

    auto submit_op = reviewsPtr->submit_review(review,
                                               [](click::Reviews::Error){});
    EXPECT_CALL(reply.instance, abort()).Times(1);
    submit_op.cancel();
}

TEST_F(ReviewsTest, testSubmitReviewUtf8)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    click::Review review;
    review.rating = 3;
    review.review_text = "'\"小海嚴選";
    review.package_name = "com.example.test";
    review.package_version = "0.1";

    // NOTE: gmock replaces the \" above as \\\" for HasSubstr(), so we have
    // to manually copy the string into the expected result.
    std::string expected_review_text = "\"review_text\":\"'\\\"小海嚴選\"";

    EXPECT_CALL(*clientPtr, callImpl(_, "POST", true, _,
                                     HasSubstr(expected_review_text), _))
            .Times(1)
            .WillOnce(Return(response));

    auto submit_op = reviewsPtr->submit_review(review,
                                               [](click::Reviews::Error){});
}

TEST_F(ReviewsTest, testSubmitReviewLanguageCorrect)
{
    LifetimeHelper<click::network::Reply, MockNetworkReply> reply;
    auto response = responseForReply(reply.asSharedPtr());

    click::Review review;
    review.rating = 3;
    review.review_text = "A review.";
    review.package_name = "com.example.test";
    review.package_version = "0.1";

    ASSERT_EQ(setenv(click::Configuration::LANGUAGE_ENVVAR,
                     "zh_TW.UTF-8", 1), 0);
    std::string expected_language = "\"language\":\"zh\"";
    EXPECT_CALL(*clientPtr, callImpl(_, "POST", true, _,
                                     HasSubstr(expected_language), _))
            .Times(1)
            .WillOnce(Return(response));

    auto submit_op = reviewsPtr->submit_review(review,
                                               [](click::Reviews::Error){});
    ASSERT_EQ(unsetenv(click::Configuration::LANGUAGE_ENVVAR), 0);
}

TEST_F(ReviewsTest, testGetBaseUrl)
{
    const char *value = getenv(click::REVIEWS_BASE_URL_ENVVAR.c_str());
    if (value != NULL) {
        ASSERT_TRUE(unsetenv(click::REVIEWS_BASE_URL_ENVVAR.c_str()) == 0);
    }
    ASSERT_TRUE(click::Reviews::get_base_url() == click::REVIEWS_BASE_URL);
    
}

TEST_F(ReviewsTest, testGetBaseUrlFromEnv)
{
    ASSERT_TRUE(setenv(click::REVIEWS_BASE_URL_ENVVAR.c_str(),
                       FAKE_SERVER.c_str(), 1) == 0);
    ASSERT_TRUE(click::Reviews::get_base_url() == FAKE_SERVER);
    ASSERT_TRUE(unsetenv(click::REVIEWS_BASE_URL_ENVVAR.c_str()) == 0);
}
