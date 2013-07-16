using Assertions;

public class ClickPackagesTestCase
{
    public static void test_parse_search_result ()
    {
        try {
            //var apps = click_service.search ("fake_query");
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
            assert_cmpstr (app.app_id, OperatorType.EQUAL, "2");
            assert_cmpstr (app.title, OperatorType.EQUAL, "MyPackage2");
            assert_cmpstr (app.price, OperatorType.EQUAL, "0");
            assert_cmpstr (app.icon_url, OperatorType.EQUAL, "http://assets.ubuntu.com/sites/ubuntu/504/u/img/ubuntu/features/icon-photos-and-videos-64x64.png");
        } catch (GLib.Error e)
        {
            assert_not_reached ();
        }
    }

    public static void test_parse_app_details ()
    {
        try {
            var details = new AppDetails.from_json (FAKE_JSON_PACKAGE_DETAILS);
            assert_cmpuint (details.binary_filesize, OperatorType.EQUAL, 123456);
            assert_cmpstr (details.keywords[1], OperatorType.EQUAL, "awesome");
            assert_cmpstr (details.description, OperatorType.EQUAL, "This application does magic.");
            assert_cmpstr (details.main_screenshot_url, OperatorType.EQUAL,
                "http://software-center.ubuntu.com/site_media/appmedia/2012/09/Screen_03.jpg");
            assert_cmpstr (details.more_screenshot_urls[1], OperatorType.EQUAL,
                "http://software-center.ubuntu.com/site_media/appmedia/2012/09/Screen_02.jpg");
            assert_cmpstr (details.download_url, OperatorType.EQUAL,
                "https://jenkins.qa.ubuntu.com/job/dropping-letters-click/4/artifact/com.ubuntu.dropping-letters_0.1.2.2_all.click");
        } catch (GLib.Error e)
        {
            assert_not_reached ();
        }
    }

    public static int main (string[] args)
    {
        Test.init (ref args);
        Test.add_data_func ("/Unit/ClickPackagesChecker/Test_Parse_Search_Result", test_parse_search_result);
        Test.add_data_func ("/Unit/ClickPackagesChecker/Test_Parse_Search_Result_Item", test_parse_search_result_item);
        Test.add_data_func ("/Unit/ClickPackagesChecker/Test_Parse_App_Details", test_parse_app_details);
        return Test.run ();
    }
}
