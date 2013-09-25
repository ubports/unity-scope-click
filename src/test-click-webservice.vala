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

using Assertions;

public class ClickTestCase
{
    public static bool run_with_timeout (MainLoop ml, uint timeout_ms)
    {
        bool timeout_reached = false;
        var t_id = Timeout.add (timeout_ms, () => {
            timeout_reached = true;
            debug ("Timeout reached");
            ml.quit ();
            return false;
        });

        ml.run ();

        if (!timeout_reached) Source.remove (t_id);

        return !timeout_reached;
    }

    public static void test_parse_search_result ()
    {
        try {
            var apps = new AvailableApps.from_json (FAKE_JSON_SEARCH_RESULT);
            assert_cmpuint (apps.size, OperatorType.EQUAL, 3);
        } catch (GLib.Error e)
        {
            assert_not_reached ();
        }
    }

    public static void test_parse_search_result_item ()
    {
        try {
            var app = new AvailableApps.from_json (FAKE_JSON_SEARCH_RESULT)[1];
            assert_cmpstr (app.app_id, OperatorType.EQUAL, "org.example.fantastiqueapp");
            assert_cmpstr (app.title, OperatorType.EQUAL, "Fantastic App");
            assert_cmpstr (app.price, OperatorType.EQUAL, "0");
            assert_cmpstr (app.icon_url, OperatorType.EQUAL, "http://assets.ubuntu.com/sites/ubuntu/504/u/img/ubuntu/features/icon-find-more-apps-64x64.png");
        } catch (GLib.Error e)
        {
            assert_not_reached ();
        }
    }

    public static void test_parse_app_details ()
    {
        try {
            var details = new AppDetails.from_json (FAKE_JSON_PACKAGE_DETAILS);
            assert_cmpuint (details.binary_filesize, OperatorType.EQUAL, 23456);
            assert_cmpstr (details.keywords[1], OperatorType.EQUAL, "fantastic application");
            assert_cmpstr (details.description, OperatorType.EQUAL, "This application contains values for all the fields in the Solr schema.");
            assert_cmpstr (details.main_screenshot_url, OperatorType.EQUAL,
                "http://software-center.ubuntu.com/site_media/appmedia/org.example.full_app/screenshots/ss1.png");
            assert_cmpstr (details.more_screenshot_urls[1], OperatorType.EQUAL,
                "http://software-center.ubuntu.com/site_media/appmedia/org.example.full_app/screenshots/ss2.png");
            assert_cmpstr (details.download_url, OperatorType.EQUAL,
                "http://software-center.ubuntu.com/org.example.full_app/full_app-0.1.1.tar.gz");
        } catch (GLib.Error e)
        {
            assert_not_reached ();
        }
    }

    public static void test_parse_skinny_details ()
    {
        try {
            var details = new AppDetails.from_json (SKINNY_PACKAGE_DETAILS);
            assert_cmpuint (details.binary_filesize, OperatorType.EQUAL, 177582);
            assert_cmpuint (details.keywords.length, OperatorType.EQUAL, 0);
            assert_cmpuint (details.more_screenshot_urls.length, OperatorType.EQUAL, 0);
        } catch (GLib.Error e)
        {
            assert_not_reached ();
        }
    }

    public static void test_download_manager ()
    {
        HashTable<string, string> credentials = new HashTable<string, string> (str_hash, str_equal);
        credentials["consumer_key"] = "...";
        credentials["consumer_secret"] = "...";
        credentials["token"] = "...";
        credentials["token_secret"] = "...";

        var sd = new SignedDownload (credentials);
        var url = "http://alecu.com.ar/test/click/demo.php";
        var app_id = "org.example.fake.app";

        Download download = null;


        MainLoop mainloop = new MainLoop ();
        sd.start_download.begin(url, app_id, (obj, res) => {
            try {
                var download_object_path = sd.start_download.end (res);
                debug ("download created for %s", url);
                download = get_download (download_object_path);

                download.started.connect( (success) => {
                    debug ("Download started");
                });
                download.finished.connect( (error) => {
                    debug ("Download finished");
                    mainloop.quit ();
                });
                download.error.connect( (error) => {
                    debug ("Download errored");
                    mainloop.quit ();
                });
                download.progress.connect( (received, total) => {
                    debug ("Download progressing: %llu/%llu", received, total);
                });
                debug ("Download starting");
                download.start ();
            } catch (GLib.Error e) {
                error ("Can't start download: %s", e.message);
            }
        });
        assert (run_with_timeout (mainloop, 60000));

        debug ("actually starting download");
    }

    public static void test_fetch_credentials ()
    {
        MainLoop mainloop = new MainLoop ();
        var u1creds = new UbuntuoneCredentials ();

        u1creds.get_credentials.begin((obj, res) => {
            mainloop.quit ();
            try {
                var creds = u1creds.get_credentials.end (res);
                debug ("token: %s", creds["token"]);
            } catch (GLib.Error e) {
                error ("Can't fetch credentials: %s", e.message);
            }
        });
        assert (run_with_timeout (mainloop, 10000));
    }

    public static void test_click_interface ()
    {
        MainLoop mainloop = new MainLoop ();
        var click_if = new ClickInterface ();

        click_if.get_installed.begin("", (obj, res) => {
            mainloop.quit ();
            try {
                var installed = click_if.get_installed.end (res);
                debug ("first installed: %s", installed[0].title);
            } catch (GLib.Error e) {
                error ("Can't get list of installed click packages %s", e.message);
            }
        });
        assert (run_with_timeout (mainloop, 10000));
    }

    class FakeClickInterface : ClickInterface {
        Json.Parser parser;
        public override async List<unowned Json.Node> get_manifests () throws ClickError {
            parser = new Json.Parser ();
            parser.load_from_data (FAKE_APP_MANIFEST);
            return parser.get_root().get_array().get_elements();
        }
    }

    public static void test_click_get_versions ()
    {
        MainLoop mainloop = new MainLoop ();
        var click_if = new FakeClickInterface ();
        Gee.Map<string,string> versions = null;

        click_if.get_versions.begin((obj, res) => {
            mainloop.quit ();
            try {
                versions = click_if.get_versions.end (res);
                debug ("got versions");
            } catch (GLib.Error e) {
                error ("Can't get versions: %s", e.message);
            }
        });
        assert (run_with_timeout (mainloop, 10000));
        assert_cmpstr (versions["com.ubuntu.ubuntu-weather"], OperatorType.EQUAL, "0.2");
        assert_cmpstr (versions["com.ubuntu.developer.pedrocan.evilapp"], OperatorType.EQUAL, "0.4");
    }

    public static void test_click_get_dotdesktop ()
    {
        MainLoop mainloop = new MainLoop ();
        var click_if = new ClickInterface ();

        click_if.get_dotdesktop.begin("com.ubuntu.ubuntu-weather", (obj, res) => {
            mainloop.quit ();
            try {
                var dotdesktop = click_if.get_dotdesktop.end (res);
                debug ("got dotdesktop: %s", dotdesktop);
            } catch (GLib.Error e) {
                error ("Can't get dotdesktop: %s", e.message);
            }
        });
        assert (run_with_timeout (mainloop, 10000));
    }

    public static void test_available_apps ()
    {
        MainLoop mainloop = new MainLoop ();
        var click_ws = new ClickWebservice ();

        click_ws.search.begin("a*", (obj, res) => {
            mainloop.quit ();
            try {
                var available_apps = click_ws.search.end (res);
                debug ("first available app: %s", available_apps[0].title);
            } catch (GLib.Error e) {
                error ("Can't get list of available click packages: %s", e.message);
            }
        });
        assert (run_with_timeout (mainloop, 10000));
    }

    public static void test_app_details ()
    {
        MainLoop mainloop = new MainLoop ();
        var click_ws = new ClickWebservice ();

        click_ws.get_details.begin("org.example.full_app", (obj, res) => {
            mainloop.quit ();
            try {
                var app_details = click_ws.get_details.end (res);
                debug ("download_url: %s", app_details.download_url);
            } catch (GLib.Error e) {
                error ("Can't get details for a click package: %s", e.message);
            }
        });
        assert (run_with_timeout (mainloop, 10000));
    }

    public static int main (string[] args)
    {
        Test.init (ref args);
        Test.add_data_func ("/Unit/ClickChecker/Test_Click_Get_Versions", test_click_get_versions);
        Test.add_data_func ("/Unit/ClickChecker/Test_Click_Interface", test_click_interface);
        Test.add_data_func ("/Unit/ClickChecker/Test_Click_Get_DotDesktop", test_click_get_dotdesktop);
        Test.add_data_func ("/Unit/ClickChecker/Test_Parse_Search_Result", test_parse_search_result);
        Test.add_data_func ("/Unit/ClickChecker/Test_Parse_Search_Result_Item", test_parse_search_result_item);
        Test.add_data_func ("/Unit/ClickChecker/Test_Parse_App_Details", test_parse_app_details);
        Test.add_data_func ("/Unit/ClickChecker/Test_Parse_Skinny_Details", test_parse_skinny_details);
        Test.add_data_func ("/Unit/ClickChecker/Test_Download_Manager", test_download_manager);
        Test.add_data_func ("/Unit/ClickChecker/Test_Fetch_Credentials", test_fetch_credentials);
        Test.add_data_func ("/Unit/ClickChecker/Test_Available_Apps", test_available_apps);
        Test.add_data_func ("/Unit/ClickChecker/Test_App_Details", test_app_details);
        return Test.run ();
    }
}
