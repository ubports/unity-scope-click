using Assertions;

const string FAKE_JSON_SEARCH_RESULT = """
{
  "responseHeader":{
    "status":0,
    "QTime":1,
    "params":{
      "fl": "id,title,price,icon_url",
      "indent":"on",
      "start":"0",
      "q":"category:featured",
      "wt":"json",
      "version":"2.2",
      "rows":"10"}},
  "response":{"numFound":10,"start":0,"docs":[
      {
        "id":"1",
        "title":"MyPackage1",
        "price": 0.0,
        "icon_url": "http://example.org/media/apps/mypackage1/icons/icon64.png"},
      {
        "id":"2",
        "title":"MyPackage2",
        "price": 0.0,
        "icon_url": "http://example.org/media/apps/mypackage2/icons/icon64.png"},
      {
        "id":"3",
        "title":"MyPackage3",
        "price": 0.0,
        "icon_url": "http://example.org/media/apps/mypackage3/icons/icon64.png"}
    ]
  }
}
""";

const string FAKE_JSON_PACKAGE_DETAILS = """
{
  "responseHeader":{
    "status":0,
    "QTime":1,
    "params":{
      "indent":"on",
      "q":"id:11",
      "wt":"json"}},
  "response":{"numFound":1,"start":0,"docs":[
      {
        "package_name":"org.example.fantasticapp",
        "requires_license_key":true,
        "screenshot_url":"http://example.org/media/fantasticapp/screenshots/screenshot_main.png",
        "terms_of_service":"Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.",
        "id":"11",
        "title":"The Fantastic Application",
        "support_url":"http://support.example.org/fantasticapp",
        "icon_url":"http://example.org/media/fantasticapp/icons/icon64.png",
        "binary_filesize":123456,
        "version":"1.2.3",
        "company_name":"Fantastic Company",
        "website":"http://example.org/fantasticapp/",
        "description":"This application does magic.",
        "price":0.0,
        "date_published":"2013-06-01T01:23:45.678Z",
        "license":"AGPL3",
        "click_updown_url":"http://click.ubuntu.com/org.example.fantasticapp/fantasticapp-1.2.3.tar.gz",
        "click_version":"0.1",
        "license_key_path":"config/license.txt",
        "screenshot_urls":[
          "http://example.org/media/fantasticapp/screenshots/screenshot_main.png",
          "http://example.org/media/fantasticapp/screenshots/screenshot_1.png",
          "http://example.org/media/fantasticapp/screenshots/screenshot_2.png"],
        "keywords":[
          "fantastic",
          "awesome",
          "app"],
        "video_urls":[
          "http://example.org/media/fantasticapp/videos/video1.ogv",
          "http://example.org/media/fantasticapp/videos/video2.ogv"],
        "click_framework":["ubuntu-sdk-13.10"]}]
  }
}
""";


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
            assert_cmpstr (app.icon_url, OperatorType.EQUAL, "http://example.org/media/apps/mypackage2/icons/icon64.png");
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
                "http://example.org/media/fantasticapp/screenshots/screenshot_main.png");
            assert_cmpstr (details.more_screenshot_urls[1], OperatorType.EQUAL,
                "http://example.org/media/fantasticapp/screenshots/screenshot_1.png");
            assert_cmpstr (details.download_url, OperatorType.EQUAL,
                "http://click.ubuntu.com/org.example.fantasticapp/fantasticapp-1.2.3.tar.gz");
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
