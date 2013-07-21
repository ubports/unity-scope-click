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


class App : GLib.Object
{
    AppState state;
    public string app_id { get; construct; }
    public string title { get; construct; }
    public string price { get; construct; }
    public string icon_url { get; construct; }

    string? cmdline;  // NULL if not installed
    bool was_lauched; // to show the NEW emblem

    void install() {
    }
    void launch() {
    }
    void uninstall() {
    }
    public async AppDetails? fetch_details() {
        return null;
    }
    public App.from_json (Json.Object json)
    {
        Object(
            app_id: json.get_string_member("name"),
            icon_url: json.get_string_member("icon_url"),
            title: json.get_string_member("title"),
            price: json.get_double_member("price").to_string()
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

    Gee.List<string> screenshot_urls;
    public async Gee.List<Review>? getReviews() {
        return null;
    }

    public async void addReview(float rating, string review) {
    }

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

    public AppDetails.from_json (string json_string)
    {
        var parser = new Json.Parser();
        parser.load_from_data(json_string, -1);
        var details = parser.get_root().get_object();

        Object(
            app_id: details.get_string_member("name"),
            icon_url: details.get_string_member("icon_url"),
            download_url: details.get_string_member("download_url"),
            main_screenshot_url: details.get_string_member("screenshot_url"),
            license: details.get_string_member("license"),
            binary_filesize: details.get_int_member("binary_filesize"),
            more_screenshot_urls: parse_string_list (details, "screenshot_urls"),

            title: details.get_string_member("title"),
            description: details.get_string_member("description"),
            keywords: parse_string_list (details, "keywords")
        );
    }
}


class Review
{
    string title;
    float rating;
    string user;
    string body;
    int timestamp;
}

class AppList
{
}

class InstalledApps : AppList
{
}


class AvailableApps : Gee.ArrayList<App>
{
    public AvailableApps.from_json (string json_string) throws GLib.Error
    {
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
    private const string SEARCH_URL = "https://search.apps.staging.ubuntu.com/api/v1/search?q=%s";
    private const string DETAILS_URL = "https://search.apps.staging.ubuntu.com/api/v1/package/%s";

    internal Soup.SessionAsync http_session;

    construct {
        http_session = WebClient.get_webclient ();
    }

    public async AvailableApps search(string query) {
        string url = SEARCH_URL.printf(query);
        string response = "[]";
        debug ("calling %s", url);

        var message = new Soup.Message ("GET", url);
        http_session.queue_message (message, (http_session, message) => {
            if (message.status_code != Soup.KnownStatusCode.OK) {
                debug ("Web request failed: HTTP %u %s",
                       message.status_code, message.reason_phrase);
                //error = new PurchaseError.PURCHASE_ERROR (message.reason_phrase);
            } else {
                message.response_body.flatten ();
                response = (string) message.response_body.data;
                debug ("response is %s", response);
            }
            search.callback ();
        });
        yield;
        return new AvailableApps.from_json (response);
    }

    public async AppDetails get_details(string app_name) {
        string url = DETAILS_URL.printf(app_name);
        string response = "{}";
        debug ("calling %s", url);

        var message = new Soup.Message ("GET", url);
        http_session.queue_message (message, (http_session, message) => {
            if (message.status_code != Soup.KnownStatusCode.OK) {
                debug ("Web request failed: HTTP %u %s",
                       message.status_code, message.reason_phrase);
            } else {
                message.response_body.flatten ();
                response = (string) message.response_body.data;
                debug ("response is %s", response);

            }
            get_details.callback ();
        });
        yield;
        return new AppDetails.from_json (response);
    }
}
