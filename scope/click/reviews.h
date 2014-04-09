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

#ifndef CLICK_REVIEWS_H
#define CLICK_REVIEWS_H

#include <functional>
#include <string>
#include <vector>

#include "index.h"
#include "webclient.h"

namespace click
{

const std::string REVIEWS_BASE_URL_ENVVAR = "U1_REVIEWS_BASE_URL";
const std::string REVIEWS_BASE_URL = "https://reviews.ubuntu.com";
const std::string REVIEWS_API_PATH = "/click/api/1.0/reviews/";
const std::string REVIEWS_QUERY_ARGNAME = "package_name";

struct Review
{
    uint32_t    id;
    int         rating;
    uint32_t    usefulness_favorable;
    uint32_t    usefulness_total;
    bool        hide;
    std::string date_created;
    std::string date_deleted;
    std::string package_name;
    std::string package_version;
    std::string language;
    std::string summary;
    std::string review_text;
    std::string reviewer_name;
    std::string reviewer_username;
};

typedef std::vector<Review> ReviewList;

ReviewList review_list_from_json (const std::string& json);

class Reviews
{
protected:
    QSharedPointer<web::Client> client;
public:
    enum class Error {NoError, CredentialsError, NetworkError};
    Reviews(const QSharedPointer<click::web::Client>& client);
    virtual ~Reviews();

    click::web::Cancellable fetch_reviews (const std::string& package_name,
                                           std::function<void(ReviewList, Error)> callback);

    static std::string get_base_url ();
};

bool operator==(const Review& lhs, const Review& rhs);

} // namespace click

#endif // CLICK_REVIEWS_H
