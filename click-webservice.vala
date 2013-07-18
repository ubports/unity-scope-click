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

    static string[] parse_string_list (Json.Array json_array)
    {
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
        var details = parser.get_root().get_array().get_object_element(0);

        Object(
            app_id: details.get_string_member("name"),
            icon_url: details.get_string_member("icon_url"),
            download_url: details.get_string_member("download_url"),
            main_screenshot_url: details.get_string_member("screenshot_url"),
            license: details.get_string_member("license"),
            binary_filesize: details.get_int_member("binary_filesize"),
            more_screenshot_urls: parse_string_list (details.get_array_member ("screenshot_urls")),

            title: details.get_string_member("title_en"),  // TODO: remove _en
            description: details.get_string_member("description_en"),  // TODO: remove _en
            keywords: parse_string_list (details.get_array_member ("keywords_en"))  // TODO: remove _en
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

class ClickService
{
    public Gee.List<App> search (string query)
    {
        return new Gee.ArrayList<App>();
    }
}
