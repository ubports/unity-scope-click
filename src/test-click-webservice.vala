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

    public static void test_click_interface ()
    {
        MainLoop mainloop = new MainLoop ();
        var click_if = new ClickInterface ();

        click_if.get_installed.begin("", (obj, res) => {
            mainloop.quit ();
            try {
                var installed = click_if.get_installed.end (res);
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

    public static void test_click_architecture ()
    {
        try {
            var arch = ClickInterface.get_arch();
            debug("Got arch: %s", arch);
            assert_cmpint (arch.length, OperatorType.GREATER_THAN, 1);
        } catch (GLib.Error e) {
            assert_not_reached ();
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

    public static void test_click_get_dotdesktop ()
    {
        MainLoop mainloop = new MainLoop ();
        var click_if = new ClickInterface ();
        var real_path = GLib.Environment.get_variable("PATH");
        GLib.Environment.set_variable("PATH", "test-extras", true);

        click_if.get_dotdesktop.begin("com.example.corner-weather", (obj, res) => {
            mainloop.quit ();
            try {
                var dotdesktop = click_if.get_dotdesktop.end (res);
                debug ("got dotdesktop: %s", dotdesktop);
            } catch (GLib.Error e) {
                error ("Can't get dotdesktop: %s", e.message);
            }
        });
        assert (run_with_timeout (mainloop, 10000));
        GLib.Environment.set_variable("PATH", real_path, true);
    }

    public static Unity.ScopeResult create_fake_result () {
        uint fake_category = 0;
        var fake_result_type = Unity.ResultType.DEFAULT;
        var fake_metadata = new HashTable<string, Variant> (str_hash, str_equal);
        fake_metadata.insert(METADATA_APP_ID, new GLib.Variant.string("fake_app_id"));
        fake_metadata.insert(METADATA_PRICE, new GLib.Variant.string("0"));
        return Unity.ScopeResult.create("fake_uri", "fake_icon_hint", fake_category,
                                fake_result_type, "fake_mimetype", "fake_title",
                                "fake_comment", "fake_dnd_uri", fake_metadata);
    }

    class FakeClickScope : ClickScope {
        public bool preview_is_installing = false;
        protected async override Unity.Preview build_installing_preview (string app_id, string progress_source) {
            preview_is_installing = true;
            var fake_icon = null;
            var fake_screenshot = null;
            var preview = new Unity.ApplicationPreview ("fake_title", "fake_subtitle", "fake_description", fake_icon, fake_screenshot);
            return preview;
        }
    }

    public static void test_scope_in_progress ()
    {
        MainLoop mainloop = new MainLoop ();
        var scope = new FakeClickScope ();
        var result = create_fake_result ();
        var metadata = new Unity.SearchMetadata();
        string action_id = null;

        assert (!scope.preview_is_installing);
        scope.activate_async.begin(result, metadata, action_id, (obj, res) => {
            mainloop.quit ();
            try {
                var response = scope.activate_async.end (res);
            } catch (GLib.Error e) {
                error ("Can't get dotdesktop: %s", e.message);
            }
        });
        assert (run_with_timeout (mainloop, 10000));
        assert (scope.preview_is_installing);
    }

    public static int main (string[] args)
    {
        Test.init (ref args);
        /*
        Test.add_data_func ("/Unit/ClickChecker/Test_Click_Architecture", test_click_architecture);
        Test.add_data_func ("/Unit/ClickChecker/Test_Click_Get_Versions", test_click_get_versions);
        Test.add_data_func ("/Unit/ClickChecker/Test_Click_Interface", test_click_interface);
        Test.add_data_func ("/Unit/ClickChecker/Test_Parse_Search_Result", test_parse_search_result);
        Test.add_data_func ("/Unit/ClickChecker/Test_Parse_Search_Result_Item", test_parse_search_result_item);
        Test.add_data_func ("/Unit/ClickChecker/Test_Parse_App_Details", test_parse_app_details);
        Test.add_data_func ("/Unit/ClickChecker/Test_Parse_Skinny_Details", test_parse_skinny_details);
        Test.add_data_func ("/Unit/ClickChecker/Test_Available_Apps", test_available_apps);
        Test.add_data_func ("/Unit/ClickChecker/Test_Click_GetDotDesktop", test_click_get_dotdesktop);
        */
        Test.add_data_func ("/Unit/ClickChecker/Test_Scope_InProgress", test_scope_in_progress);
        return Test.run ();
    }
}
