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

using Gee;

enum AppState {
    AVAILABLE, INSTALLING, INSTALLED
}

const string JSON_FIELD_NAME = "name";
const string JSON_FIELD_ICON_URL = "icon_url";
const string JSON_FIELD_TITLE = "title";
const string JSON_FIELD_PRICE = "price";
const string JSON_FIELD_DOWNLOAD_URL = "download_url";
const string JSON_FIELD_SCREENSHOT_URL = "screenshot_url";
const string JSON_FIELD_LICENSE = "license";
const string JSON_FIELD_BINARY_FILESIZE = "binary_filesize";
const string JSON_FIELD_SCREENSHOT_URLS = "screenshot_urls";
const string JSON_FIELD_DESCRIPTION = "description";
const string JSON_FIELD_KEYWORDS = "keywords";

errordomain WebserviceError {
    HTTP_ERROR,
    JSON_ERROR
}


class App : GLib.Object
{
    AppState state;
    public string app_id { get; construct; }
    public string title { get; construct; }
    public string price { get; construct; }
    public string icon_url { get; construct; }

    //string? cmdline;  // NULL if not installed
    //bool was_lauched; // to show the NEW emblem

    /*
    void install() {
    }
    void launch() {
    }
    void uninstall() {
    }
    public async AppDetails? fetch_details() {
        return null;
    }
    */
    public App.from_json (Json.Object json)
    {
        Object(
            app_id: json.get_string_member(JSON_FIELD_NAME),
            icon_url: json.get_string_member(JSON_FIELD_ICON_URL),
            title: json.get_string_member(JSON_FIELD_TITLE),
            price: json.get_double_member(JSON_FIELD_PRICE).to_string()
        );
        state = AppState.AVAILABLE;
    }
}


class AppDetails : GLib.Object
{
    public string app_id { get; construct; }
    public string icon_url { get; construct; }
    public string title { get; construct; }
    public string description { get; construct; } // NULL if not purchased
    public string download_url { get; construct; } // NULL if not purchased
    public float rating { get; construct; } // 0.0-1.0, shown as 5 stars?
    public string[] keywords { get; construct; }
    public string terms_of_service { get; construct; }
    public string package_name { get; construct; }
    public string license { get; construct; }
    public string main_screenshot_url { get; construct; }
    public string[] more_screenshot_urls { get; construct; }
    public uint64 binary_filesize { get; construct; }


    /* TODO: use RnR webservice
    public async Gee.List<Review>? getReviews() {
        return null;
    }

    public async void addReview(float rating, string review) {
    }
    */

    static string[] parse_string_list (Json.Object parent, string array_name)
    {
        if (!parent.has_member(array_name)) {
            return new string[0];
        }
        var json_array = parent.get_array_member(array_name);
        var ret = new string[json_array.get_length ()];
        int n = 0;
        foreach (var node in json_array.get_elements ()) {
            ret[n++] = node.dup_string ();
        }
        return ret;
    }

    public AppDetails.from_json (string json_string) throws GLib.Error
    {
        var parser = new Json.Parser();
        parser.load_from_data(json_string, -1);
        var details = parser.get_root().get_object();

        Object(
            app_id: details.get_string_member(JSON_FIELD_NAME),
            icon_url: details.get_string_member(JSON_FIELD_ICON_URL),
            download_url: details.get_string_member(JSON_FIELD_DOWNLOAD_URL),
            main_screenshot_url: details.get_string_member(JSON_FIELD_SCREENSHOT_URL),
            license: details.get_string_member(JSON_FIELD_LICENSE),
            binary_filesize: details.get_int_member(JSON_FIELD_BINARY_FILESIZE),
            more_screenshot_urls: parse_string_list (details, JSON_FIELD_SCREENSHOT_URLS),

            title: details.get_string_member(JSON_FIELD_TITLE),
            description: details.get_string_member(JSON_FIELD_DESCRIPTION),
            keywords: parse_string_list (details, JSON_FIELD_KEYWORDS)
        );
    }
}


/* TODO: use RnR webservice
class Review
{
    string title;
    float rating;
    string user;
    string body;
    int timestamp;
}
*/

class AppList
{
}

class InstalledApps : AppList
{
}


class AvailableApps : Gee.ArrayList<App> {
    public AvailableApps.from_json (string json_string) throws GLib.Error {
        var parser = new Json.Parser();
        parser.load_from_data(json_string, -1);
        var docs = parser.get_root().get_array();
        foreach (var document in docs.get_elements()) {
            add (new App.from_json (document.get_object()));
        }
    }
}


class InstallingApps : AppList
{
}

class WebClient : GLib.Object {
    static Soup.SessionAsync http_session = null;
    private const string USER_AGENT = "UnityScopeClick/0.1 (libsoup)";

    public static Soup.SessionAsync get_webclient () {
        if (http_session == null) {
            http_session = new Soup.SessionAsync ();
            http_session.user_agent = USER_AGENT;
        }
        return http_session;
    }
}

class ClickWebservice : GLib.Object
{
    private const string SEARCH_BASE_URL = "https://search.apps.ubuntu.com/";
    private const string SEARCH_PATH = "api/v1/search?q=%s";
    private const string DETAILS_PATH = "api/v1/package/%s";

    internal Soup.SessionAsync http_session;

    construct {
        http_session = WebClient.get_webclient ();
    }

    string from_environ (string env_name, string default_value) {
        string env_value = Environment.get_variable (env_name);
        return env_value != null ? env_value : default_value;
    }

    string get_base_url () {
        return from_environ ("U1_SEARCH_BASE_URL", SEARCH_BASE_URL);
    }

    string get_search_url() {
        return get_base_url() + SEARCH_PATH;
    }

    string get_details_url() {
        return get_base_url() + DETAILS_PATH;
    }

    public async AvailableApps search(string query) throws WebserviceError {
        WebserviceError failure = null;
        string url = get_search_url().printf(query);
        string response = "[]";
        debug ("calling %s", url);

        var message = new Soup.Message ("GET", url);
        http_session.queue_message (message, (http_session, message) => {
            if (message.status_code != Soup.KnownStatusCode.OK) {
                var msg = "Web request failed: HTTP %u %s".printf(
                       message.status_code, message.reason_phrase);
                failure = new WebserviceError.HTTP_ERROR(msg);
            } else {
                message.response_body.flatten ();
                response = (string) message.response_body.data;
                debug ("response is %s", response);
            }
            search.callback ();
        });
        yield;
        if (failure != null) {
            throw failure;
        }
        try {
            return new AvailableApps.from_json (response);
        } catch (GLib.Error e) {
            var msg = "Error parsing json: %s".printf(e.message);
            throw new WebserviceError.HTTP_ERROR(msg);
        }
    }

    public async AppDetails get_details(string app_name) throws WebserviceError {
        WebserviceError failure = null;
        string url = get_details_url().printf(app_name);
        string response = "{}";
        debug ("calling %s", url);

        var message = new Soup.Message ("GET", url);
        http_session.queue_message (message, (http_session, message) => {
            if (message.status_code != Soup.KnownStatusCode.OK) {
                var msg ="Web request failed: HTTP %u %s".printf(
                       message.status_code, message.reason_phrase);
                failure = new WebserviceError.HTTP_ERROR(msg);
            } else {
                message.response_body.flatten ();
                response = (string) message.response_body.data;
                debug ("response is %s", response);
            }
            get_details.callback ();
        });
        yield;
        if (failure != null) {
            throw failure;
        }
        try {
            return new AppDetails.from_json (response);
        } catch (GLib.Error e) {
            var msg = "Error parsing json: %s".printf(e.message);
            throw new WebserviceError.HTTP_ERROR(msg);
        }
    }
}
