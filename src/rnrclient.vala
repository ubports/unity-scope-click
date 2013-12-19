/* -*- tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2013 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

using Json;

/**
 * RNRClient:
 *
 * Client for the ratings and reviews server.
 * It is able to change the server address by setting the environment
 * variable "U1_REVIEWS_BASE_URL".
 * 
 */ 
class RNRClient : GLib.Object
{
    UbuntuoneCredentials u1creds;
    const string CONSUMER_KEY = "consumer_key";
    const string CONSUMER_SECRET = "consumer_secret";
    const string TOKEN = "token";
    const string TOKEN_SECRET = "token_secret";
    const string REVIEWS_BASE_URL = "https://reviews.ubuntu.com/reviews";

    internal Soup.SessionAsync http_session;

    construct {
        http_session = WebClient.get_webclient();
        u1creds = new UbuntuoneCredentials ();
    }

    private string from_environ (string env_name, string default_value) {
        string env_value = Environment.get_variable (env_name);
        return env_value != null ? env_value : default_value;
    }

    private string get_base_url () {
        return from_environ ("U1_REVIEWS_BASE_URL", REVIEWS_BASE_URL);
    }

    private string get_review_url() {
        return get_base_url() + "/api/1.0/reviews/filter/%s/%s/%s/%s/%s%s/page/%s/%s/";
    }

    /**
     * get_reviews:
     * @details: the AppDetails describe the details.
     *
     * The function returns all the reviews based on the application
     * details. It only returns the reviews that matches the system
     * language.
     *
     * Returns: A Variant (list of dictionary) that containts the reviews.
     *
     */
    public async Variant? get_reviews(AppDetails details) {
        string[] languages = GLib.Intl.get_language_names();
        string? language = null;
        if (languages.length > 0) {
            language = languages[0];
        }
        var filter = new ReviewFilter();
        filter.language = language;
        filter.appname = details.title;
        filter.origin = "click";
        return yield get_reviews_by_filter(filter);
    }

    /**
     * get_reviews_by_filter:
     * @filter: the filter for reviews.
     *
     * The function returns all the reviews based on the filter.
     *
     * Returns: A Variant (list of dictionary) that containts the reviews.
     *
     */
    public async Variant? get_reviews_by_filter(ReviewFilter filter) {
        Variant? ret = null;
        WebserviceError failure = null;
        string url = get_review_url().printf(filter.language,
                                             filter.origin,
                                             filter.distroseries,
                                             filter.version,
                                             filter.packagename,
                                             filter.appname,
                                             filter.page,
                                             filter.sort);
        string response="";

        debug ("url is %s", url);

        var message = new Soup.Message ("GET", url);
        http_session.queue_message(message, (http_session, message) => {
            if (message.status_code != Soup.KnownStatusCode.OK) {
                var msg = "Web request failed: HTTP %u %s".printf(
                       message.status_code, message.reason_phrase);
                failure = new WebserviceError.HTTP_ERROR(msg);
            } else {
                message.response_body.flatten ();
                response = (string) message.response_body.data;
                debug ("response is %s", response);
            }
            get_reviews_by_filter.callback();
        });
        yield;
        if (failure != null) {
            debug("cannot get reviews error %s", failure.message);
            return ret;
        }
        var parser = new Json.Parser();
        try {
            parser.load_from_data(response);
        } catch (GLib.Error e) {
            debug ("parse reviews response error %s", e.message);
            return ret;
        }

        ret = convertJSONtoVariant(parser);
        return ret;
    }

    protected Variant? convertJSONtoVariant(Json.Parser parser) {
        Variant? ret = null;
        var root_object = parser.get_root();
        var builder = new VariantBuilder(new VariantType("av"));

        foreach (var node in root_object.get_array().get_elements()) {
            var ht = new VariantBuilder(new VariantType("a{sv}"));
            var node_object = node.get_object();
            foreach (var node_member in node_object.get_members()) {
                Variant? gvar_member = null;
                try {
                    gvar_member = Json.gvariant_deserialize(node_object.get_member(node_member), null);
                } catch (GLib.Error e) {
                    debug ("gvariant deserialize error: %s",e.message);
                    continue;
                }
                if (gvar_member == null) {
                    continue;
                }
                if (gvar_member.is_of_type(VariantType.MAYBE) && gvar_member.get_maybe() == null) {
                    continue;
                }
                ht.add("{sv}", node_member, gvar_member);
            }
            Variant dict = ht.end();
            builder.add("v", dict);
        }

        ret = builder.end();
        return ret;
    }
}

/**
 * ReviewFilter:
 * @packagename: the name of package. Can be "" if no packagename.
 * @language: the language string. Eg: en zh ... The default value "any" to
 *            get reviews for any languages
 * @origin: Normally comes from "ubuntu". For click packages it is "click".
 *          The default value "any" to get reviews for any origin.
 * @distroseries: For example, "natty", "saucy". The default value "any" to 
 *                get any distroseries.
 * @version: The version of the software. Default value "any" means to get
 *           all versions from the server.
 * @appname: the name of application. Can be "" if no appname.
 * @page: the page number. Start from 1.
 * @sort: sort the reviews. Default is by "helpful".
 *
 * ReviewFilter is the class used to pass the reviews filter to the server
 * API to get the reviews. The server contains many reviews with different 
 * languages, different versions, etc. We should use the filter to get the
 * reviews we want.
 * 
 */ 
class ReviewFilter : GLib.Object {
    public string packagename = "";
    public string language = "any";
    public string origin = "any";
    public string distroseries = "any";
    public string version = "any";
    public string appname = "";
    public string page = "1";
    public string sort = "helpful";
}
