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

#include <cstdlib>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>

#include <json/value.h>
#include <json/writer.h>

#include "reviews.h"

namespace click
{

bool operator== (const Review& lhs, const Review &rhs)
{
    return lhs.id == rhs.id &&
        lhs.rating == rhs.rating &&
        lhs.usefulness_favorable == rhs.usefulness_favorable &&
        lhs.usefulness_total == rhs.usefulness_total &&
        lhs.hide == rhs.hide &&
        lhs.date_created == rhs.date_created &&
        lhs.date_deleted == rhs.date_deleted &&
        lhs.package_name == rhs.package_name &&
        lhs.package_version == rhs.package_version &&
        lhs.language == rhs.language &&
        lhs.summary == rhs.summary &&
        lhs.review_text == rhs.review_text &&
        lhs.reviewer_name == rhs.reviewer_name &&
        lhs.reviewer_username == rhs.reviewer_username;
}

ReviewList review_list_from_json (const std::string& json)
{
    std::istringstream stream(json);

    boost::property_tree::ptree tree;
    boost::property_tree::read_json(stream, tree);

    ReviewList reviews;

    BOOST_FOREACH(boost::property_tree::ptree::value_type &value, tree)
    {
        assert(value.first.empty()); // array elements have no names
        auto node = value.second;
        Review review;

        review.id = node.get<uint32_t>("id");
        review.rating = node.get<int>("rating");
        review.usefulness_favorable = node.get<uint32_t>("usefulness_favorable");
        review.usefulness_total = node.get<uint32_t>("usefulness_total");
        review.hide = node.get<bool>("hide");
        review.date_created = node.get<std::string>("date_created");
        review.date_deleted = node.get<std::string>("date_deleted");
        review.package_name = node.get<std::string>("package_name");
        review.package_version = node.get<std::string>("version");
        review.language = node.get<std::string>("language");
        review.summary = node.get<std::string>("summary");
        review.review_text = node.get<std::string>("review_text");
        review.reviewer_username = node.get<std::string>("reviewer_username");
        review.reviewer_name = node.get<std::string>("reviewer_displayname", review.reviewer_username);

        reviews.push_back(review);
    }
    return reviews;
}

Reviews::Reviews (const QSharedPointer<click::web::Client>& client)
    : client(client)
{
}

Reviews::~Reviews ()
{
}

click::web::Cancellable Reviews::fetch_reviews (const std::string& package_name,
                                                std::function<void(ReviewList, Error)> callback)
{
    click::web::CallParams params;
    params.add(click::REVIEWS_QUERY_ARGNAME, package_name.c_str());
    QSharedPointer<click::web::Response> response = client->call
        (get_base_url() + click::REVIEWS_API_PATH, params);

    QObject::connect(response.data(), &click::web::Response::finished,
                [=](QString reply) {
                    click::ReviewList reviews;
                    reviews = review_list_from_json(reply.toUtf8().constData());
                    callback(reviews, click::Reviews::Error::NoError);
                });
    QObject::connect(response.data(), &click::web::Response::error,
                [=](QString) {
                    qDebug() << "Network error attempting to fetch reviews for:" << package_name.c_str();
                    callback(ReviewList(), click::Reviews::Error::NetworkError);
                });

    return click::web::Cancellable(response);
}

click::web::Cancellable Reviews::submit_review (const PackageDetails& package,
                                                int rating,
                                                const std::string& review_text)
{
    std::map<std::string, std::string> headers({
            {click::web::CONTENT_TYPE_HEADER, click::web::CONTENT_TYPE_JSON},
                });
    // TODO: Need to get language for the package/review.
    Json::Value root(Json::ValueType::objectValue);
    root["package_name"] = package.package.name;
    root["version"] = package.package.version;
    root["rating"] = rating;
    root["review_text"] = review_text;
    root["arch_tag"] = click::Configuration().get_architecture();

    // NOTE: "summary" is a required field, but we don't have one. Use "".
    root["summary"] = "";
    
    qDebug() << "Rating" << package.package.name.c_str() << rating;

    QSharedPointer<click::web::Response> response = client->call
        (get_base_url() + click::REVIEWS_API_PATH, "POST", true,
         headers, Json::FastWriter().write(root), click::web::CallParams());

    QObject::connect(response.data(), &click::web::Response::finished,
                [=](QString) {
                   qDebug() << "Review submitted for:" << package.package.name.c_str();
                });
    QObject::connect(response.data(), &click::web::Response::error,
                [=](QString) {
                    qCritical() << "Network error submitting a reviews for:" << package.package.name.c_str();
                });

    return click::web::Cancellable(response);
}

std::string Reviews::get_base_url ()
{
    const char *env_url = getenv(REVIEWS_BASE_URL_ENVVAR.c_str());
    if (env_url != NULL) {
        return env_url;
    }
    return click::REVIEWS_BASE_URL;
}

} //namespace click
