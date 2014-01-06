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

const string FAKE_APP_ID = "FAKE_APP_ID";
const string FAKE_DOT_DESKTOP = "FAKE_DOT_DESKTOP";
const string FAKE_OBJECT_PATH = "/com/fake/object";
const string FAKE_DOWNLOAD_ERROR_MESSAGE = "fake generic download error message";
const string FAKE_CREDS_ERROR_MESSAGE = "fake creds error message";
const string FAKE_INVALID_CREDS_ERROR_MESSAGE = "fake invalid creds message";

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
            assert (app.price == 0.0f);
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
        
        public override async string get_dotdesktop (string app_id) throws ClickError {
            return FAKE_DOT_DESKTOP;
        }

        public bool fake_can_uninstall = true;
        public override async bool can_uninstall (string app_id) {
            return fake_can_uninstall;
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

    class FakeCredentials : UbuntuoneCredentials {

        public bool invalidate_called = false;
        
        public override async HashTable<string, string> get_credentials () throws CredentialsError {
            string credentials = "consumer_key=consumer&consumer_secret=consumer_secret&token=token&token_secret=token_secret";
            return Soup.Form.decode (credentials);
        }

        public override async void invalidate_credentials () throws CredentialsError {
            this.invalidate_called = true;
        }
    }

    class FakeClickWebservice : ClickWebservice {
        public override async AppDetails get_details (string app_name) throws WebserviceError {
            return new AppDetails.from_json (FAKE_JSON_PACKAGE_DETAILS);
        }
    }

    class FakeSignedDownloadOK : SignedDownload {
        public async override new GLib.ObjectPath start_download (string uri, string app_id) throws DownloadError {
            return new GLib.ObjectPath (FAKE_OBJECT_PATH);
        }

        public FakeSignedDownloadOK (HashTable<string, string> credentials) {
            base (credentials);
        }
    }

    class ClickScopeWithFakeSignedDownloadOK : ClickScope {
        protected override SignedDownload get_signed_download (HashTable<string, string> credentials) {
            return new FakeSignedDownloadOK (credentials);
        }
    }

    public static void test_install_app_ok ()
    {    
        MainLoop mainloop = new MainLoop ();

        var scope = new ClickScopeWithFakeSignedDownloadOK ();
        scope.click_if = new FakeClickInterface ();
        scope.u1creds = new FakeCredentials ();
        scope.webservice = new FakeClickWebservice ();

        scope.install_app.begin (FAKE_APP_ID, (obj, res) => {
             mainloop.quit ();
             try {

                 var objpath = scope.install_app.end (res);
                 assert (objpath == FAKE_OBJECT_PATH);

             } catch (GLib.Error e) {
                 error ("Caught GLib.Error in test_install_app_ok: %s", e.message);
             }
         });
        assert (run_with_timeout (mainloop, 10000));
        
    }

    class FakeBadCredentials : UbuntuoneCredentials {
        public override async new HashTable<string, string> get_credentials () throws CredentialsError {
            throw new CredentialsError.CREDENTIALS_ERROR (FAKE_CREDS_ERROR_MESSAGE);
        }
    }

    public static void test_install_app_bad_creds ()
    {    
        MainLoop mainloop = new MainLoop ();

        var scope = new ClickScopeWithFakeSignedDownloadOK ();
        scope.click_if = new FakeClickInterface ();
        scope.u1creds = new FakeBadCredentials ();
        scope.webservice = new FakeClickWebservice ();

        scope.install_app.begin (FAKE_APP_ID, (obj, res) => {
             mainloop.quit ();
             try {

                 var objpath = scope.install_app.end (res);
                 assert_not_reached ();

             } catch (ClickScopeError.LOGIN_ERROR e) {
                 assert (e.message == FAKE_CREDS_ERROR_MESSAGE); 

             } catch {
                 assert_not_reached ();
             }
         });
        assert (run_with_timeout (mainloop, 10000));
    }

    class FakeSignedDownloadInvalid : SignedDownload {
        public async override new GLib.ObjectPath start_download (string uri, string app_id) throws DownloadError {
            throw new DownloadError.INVALID_CREDENTIALS (FAKE_INVALID_CREDS_ERROR_MESSAGE);
        }
        public FakeSignedDownloadInvalid (HashTable<string, string> credentials) {
            base (credentials);
        }
    }

    class ClickScopeWithFakeSignedDownloadInvalid : ClickScope {
        protected override SignedDownload get_signed_download (HashTable<string, string> credentials) {
            return new FakeSignedDownloadInvalid (credentials);
        }
    }

    public static void test_install_app_invalid_creds ()
    {    
        MainLoop mainloop = new MainLoop ();

        var scope = new ClickScopeWithFakeSignedDownloadInvalid ();
        scope.click_if = new FakeClickInterface ();
        scope.u1creds = new FakeCredentials ();
        scope.webservice = new FakeClickWebservice ();

        scope.install_app.begin (FAKE_APP_ID, (obj, res) => {
             mainloop.quit ();
             try {

                 var objpath = scope.install_app.end (res);
                 assert_not_reached ();

             } catch (ClickScopeError.LOGIN_ERROR e) {
                 assert (e.message == FAKE_INVALID_CREDS_ERROR_MESSAGE); 
                 assert (((FakeCredentials) scope.u1creds).invalidate_called == true);

             } catch {
                 assert_not_reached ();
             }
         });
        assert (run_with_timeout (mainloop, 10000));
        
    }

    class FakeSignedDownloadError : SignedDownload {
        public async override new GLib.ObjectPath start_download (string uri, string app_id) throws DownloadError {
            throw new DownloadError.DOWNLOAD_ERROR (FAKE_DOWNLOAD_ERROR_MESSAGE);
        }
        public FakeSignedDownloadError (HashTable<string, string> credentials) {
            base (credentials);
        }
    }

    class ClickScopeWithFakeSignedDownloadError : ClickScope {
        protected override SignedDownload get_signed_download (HashTable<string, string> credentials) {
            return new FakeSignedDownloadError (credentials);
        }
    }

    public static void test_install_app_download_error ()
    {    
        MainLoop mainloop = new MainLoop ();

        var scope = new ClickScopeWithFakeSignedDownloadError ();
        scope.click_if = new FakeClickInterface ();
        scope.u1creds = new FakeCredentials ();
        scope.webservice = new FakeClickWebservice ();

        scope.install_app.begin (FAKE_APP_ID, (obj, res) => {
             mainloop.quit ();
             try {

                 var objpath = scope.install_app.end (res);
                 assert_not_reached ();

             } catch (ClickScopeError.INSTALL_ERROR e) {
                 assert (e.message == FAKE_DOWNLOAD_ERROR_MESSAGE); 
                 assert (((FakeCredentials) scope.u1creds).invalidate_called == false);

             } catch {
                 assert_not_reached ();
             }
         });
        assert (run_with_timeout (mainloop, 10000));
        
    }

    public static void test_click_get_dotdesktop ()
    {
        MainLoop mainloop = new MainLoop ();
        var click_if = new FakeClickInterface ();

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

	private static bool preview_has_info_hint(Unity.Preview preview, string id, Variant val)
	{
		var serialized = preview.serialize();
		// NOTE: the signature for serialize is (ssssssa(sssua{sv})a(sssv)a{sv})
		GLib.Variant hints;
		hints = serialized.get_child_value(7); // the a(sssv) part

		foreach (var hint in hints) {
			string hint_id;
			GLib.Variant hint_val;
			hint.get_child(0, "s", out hint_id);
			hint.get_child(3, "v", out hint_val);

			if (hint_id == id && hint_val.equal(val)){
				return true;
			}
		}
		return false;
	}

	private static bool preview_has_action(Unity.Preview preview, string action_name, string action_title)
	{
		var serialized = preview.serialize();
		// NOTE: the signature for serialize is (ssssssa(sssua{sv})a(sssv)a{sv})
		GLib.Variant actions;
		actions = serialized.get_child_value(6); // the a(sssua{sv}) part

		foreach (var action in actions) {
			string name;
			string title;
			action.get_child(0, "s", out name);
			action.get_child(1, "s", out title);

			if (name == action_name && title == action_title) {
				return true;
			}
		}
		return false;
	}

    public static void test_scope_build_purchasing_preview ()
    {
        MainLoop mainloop = new MainLoop ();
        ClickScope scope = new ClickScope ();
        scope.webservice = new FakeClickWebservice ();
        var metadata = new HashTable<string, Variant> (str_hash, str_equal);
        metadata.insert(METADATA_APP_ID, new GLib.Variant.string(FAKE_APP_ID));
        var fake_result = Unity.ScopeResult.create("", "", 0,
                                                   Unity.ResultType.DEFAULT,
                                                   "application/x-desktop",
                                                   "", "", "", metadata);

        scope.build_purchasing_preview.begin(fake_result, (obj, res) => {
                mainloop.quit ();
                try {
                    var preview = scope.build_purchasing_preview.end(res);
                    assert(preview_has_info_hint(preview, "show_purchase_overlay",
                                                 new Variant.boolean(true)));
                    assert(preview_has_info_hint(preview, "package_name",
                                                 new Variant.string("FAKE_APP_ID")));
                    assert(preview_has_action(preview, "purchase_succeeded", "*** purchase_succeeded"));
                    assert(preview_has_action(preview, "purchase_failed", "*** purchase_failed"));

                } catch (GLib.Error e) {
                    error ("Exception caught building purchasing preview: %s", e.message);
                }
            });

        assert (run_with_timeout (mainloop, 10000));
    }

    public static void test_scope_build_installed_preview_with_uninstall ()
    {
        do_test_build_installed_preview_uninstall (true);
    }

    public static void test_scope_build_installed_preview_without_uninstall ()
    {
        do_test_build_installed_preview_uninstall (false);
    }
    
    private static void do_test_build_installed_preview_uninstall (bool can_uninstall)
    {
        MainLoop mainloop = new MainLoop ();
        ClickScope scope = new ClickScope ();
        scope.webservice = new FakeClickWebservice ();
        scope.click_if = new FakeClickInterface ();
        ((FakeClickInterface) scope.click_if).fake_can_uninstall = can_uninstall;

        var metadata = new HashTable<string, Variant> (str_hash, str_equal);
        metadata.insert(METADATA_APP_ID, new GLib.Variant.string(FAKE_APP_ID));
        var fake_result = Unity.ScopeResult.create("", "", 0,
                                                   Unity.ResultType.DEFAULT,
                                                   "application/x-desktop",
                                                   "", "", "", metadata);

        string fake_app_uri = "application:///" + FAKE_DOT_DESKTOP;
        scope.build_installed_preview.begin(fake_result, fake_app_uri, (obj, res) => {
                mainloop.quit ();
                try {
                    var preview = scope.build_installed_preview.end(res);
                    assert(preview_has_action(preview, "open_click:" + fake_app_uri, "Open"));
                    
                    bool has_uninstall = preview_has_action(preview, "uninstall_click", "Uninstall");
                    assert(can_uninstall == has_uninstall);

                } catch (GLib.Error e) {
                    error ("Exception caught building installed preview: %s", e.message);
                }
            });

        assert (run_with_timeout (mainloop, 10000));
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
        protected async override Unity.Preview build_installing_preview (Unity.ScopeResult result, string progress_source) {
            preview_is_installing = true;
            var fake_icon = null;
            var fake_screenshot = null;
            var preview = new Unity.ApplicationPreview ("fake_title", "fake_subtitle", "fake_description", fake_icon, fake_screenshot);
            return preview;
        }

        internal override string? get_progress_source(string app_id) {
            if (app_id == "fake_app_id") {
                return "fake_progress_source";
            }
            return null;
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
                error ("Failure in activate_async: %s", e.message);
            }
        });
        assert (run_with_timeout (mainloop, 10000));
        assert (scope.preview_is_installing);
    }

    class FakeClickScopeForBuildDefaultTest : ClickScope {
        public bool finds_progress = false;
        public bool is_installable = false;

        public bool build_installing_called = false;
        public bool build_uninstalled_called = false;
        public bool build_installed_called = false;

        protected async override Unity.Preview build_installing_preview (Unity.ScopeResult result, string progress_source) {
            build_installing_called = true;
            return new Unity.ApplicationPreview ("fake_title", "fake_subtitle", "fake_description", null, null);
        }

        protected async override Unity.Preview build_uninstalled_preview (Unity.ScopeResult result) {
            build_uninstalled_called = true;
            return new Unity.ApplicationPreview ("fake_title", "fake_subtitle", "fake_description", null, null);
        }

        protected async override Unity.Preview build_installed_preview (Unity.ScopeResult result, string application_uri) {
            build_installed_called = true;
            return new Unity.ApplicationPreview ("fake_title", "fake_subtitle", "fake_description", null, null);
        }

        internal override string? get_progress_source(string app_id) {
            if (finds_progress) {
                return "fake_progress_source";
            }
            return null;
        }

        protected override bool uri_is_click_install (string uri) {
            return is_installable;
        }
    }

    public static void test_scope_build_default_preview_finds_progress_not_installable ()
    {
        do_test_scope_build_default_preview (true, false);
    }

    public static void test_scope_build_default_preview_no_progress_source_not_installable ()
    {
        do_test_scope_build_default_preview (false, false);
    }

    public static void test_scope_build_default_preview_finds_progress_is_installable ()
    {
        do_test_scope_build_default_preview (true, true);
    }

    public static void test_scope_build_default_preview_no_progress_source_is_installable ()
    {
        do_test_scope_build_default_preview (false, true);
    }

    public static void do_test_scope_build_default_preview (bool finds_progress, bool is_installable)
    {    
        MainLoop mainloop = new MainLoop ();
        var scope = new FakeClickScopeForBuildDefaultTest ();
        scope.finds_progress = finds_progress;
        scope.is_installable = is_installable;
        var result = create_fake_result ();
        var metadata = new Unity.SearchMetadata();

        scope.build_default_preview.begin(result, (obj, res) => {
            mainloop.quit ();
            try {
                var response = scope.build_default_preview.end (res);
            } catch (GLib.Error e) {
                error ("Failure in activate_async: %s", e.message);
            }
        });
        assert (run_with_timeout (mainloop, 10000));
        assert (scope.build_installing_called == finds_progress);
        assert (scope.build_uninstalled_called == (!finds_progress && is_installable));
        assert (scope.build_installed_called == (!is_installable && !finds_progress));
    }

    public static int main (string[] args)
    {
        Test.init (ref args);
        Test.add_data_func ("/Unit/ClickChecker/Test_Click_Architecture", test_click_architecture);
        Test.add_data_func ("/Unit/ClickChecker/Test_Click_Get_Versions", test_click_get_versions);
        Test.add_data_func ("/Unit/ClickChecker/Test_Click_Interface", test_click_interface);
        Test.add_data_func ("/Unit/ClickChecker/Test_Parse_Search_Result", test_parse_search_result);
        Test.add_data_func ("/Unit/ClickChecker/Test_Parse_Search_Result_Item", test_parse_search_result_item);
        Test.add_data_func ("/Unit/ClickChecker/Test_Parse_App_Details", test_parse_app_details);
        Test.add_data_func ("/Unit/ClickChecker/Test_Parse_Skinny_Details", test_parse_skinny_details);
        Test.add_data_func ("/Unit/ClickChecker/Test_Install_App_OK", test_install_app_ok);
        Test.add_data_func ("/Unit/ClickChecker/Test_Install_App_Bad_Creds", test_install_app_bad_creds);
        Test.add_data_func ("/Unit/ClickChecker/Test_Install_App_Invalid_Creds", test_install_app_invalid_creds);
        Test.add_data_func ("/Unit/ClickChecker/Test_Install_App_Download_Error", test_install_app_download_error);
        Test.add_data_func ("/Unit/ClickChecker/Test_Click_GetDotDesktop", test_click_get_dotdesktop);
        Test.add_data_func ("/Unit/ClickChecker/Test_Scope_Build_Purchasing_Preview", 
                            test_scope_build_purchasing_preview);
        Test.add_data_func ("/Unit/ClickChecker/Test_Scope_Build_Installed_Preview_With_Uninstall", 
                            test_scope_build_installed_preview_with_uninstall);
        Test.add_data_func ("/Unit/ClickChecker/Test_Scope_Build_Installed_Preview_Without_Uninstall", 
                            test_scope_build_installed_preview_without_uninstall);
        Test.add_data_func ("/Unit/ClickChecker/Test_Scope_InProgress", test_scope_in_progress);
        Test.add_data_func ("/Unit/ClickChecker/Test_Scope_Build_Default_Preview_No_Progress_Is_Installable",
                            test_scope_build_default_preview_no_progress_source_is_installable);
        Test.add_data_func ("/Unit/ClickChecker/Test_Scope_Build_Default_Preview_No_Progress_Not_Installable",
                            test_scope_build_default_preview_no_progress_source_not_installable);
        Test.add_data_func ("/Unit/ClickChecker/Test_Scope_Build_Default_Preview_Finds_Progress_Is_Installable",
                            test_scope_build_default_preview_finds_progress_is_installable);
        Test.add_data_func ("/Unit/ClickChecker/Test_Scope_Build_Default_Preview_Finds_Progress_Not_Installable",
                            test_scope_build_default_preview_finds_progress_not_installable);
        return Test.run ();
    }
}
