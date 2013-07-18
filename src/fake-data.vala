const string FAKE_JSON_SEARCH_RESULT = """
[
  {
    "name": "org.example.awesomelauncher",
    "title": "Awesome Launcher",
    "description": "This is an awesome launcher.",
    "price": 1.99,
    "icon_url": "http://software-center.ubuntu.com/site_media/appmedia/2012/09/SPAZ.png",
    "resource_url": "http://search.apps.ubuntu.com/api/v1/package/org.example.awesomelauncher"
  },
  {
    "name": "org.example.fantastiqueapp",
    "title": "Fantastic App",
    "description": "This is a fantasticc app.",
    "price": 0.0,
    "icon_url": "http://assets.ubuntu.com/sites/ubuntu/504/u/img/ubuntu/features/icon-find-more-apps-64x64.png",
    "resource_url": "http://search.apps.ubuntu.com/api/v1/package/org.example.fantasticapp"
  },
  {
    "name": "org.example.awesomewidget",
    "title": "Awesome Widget",
    "description": "This is an awesome widget.",
    "price": 1.99,
    "icon_url": "http://assets.ubuntu.com/sites/ubuntu/504/u/img/ubuntu/features/icon-photos-and-videos-64x64.png",
    "resource_url": "http://search.apps.ubuntu.com/api/v1/package/org.example.awesomewidget"
  }
]
""";

/*
const string FAKE_JSON_PACKAGE_DETAILS = """
{
    "package_name":"org.example.fantasticapp",
    "requires_license_key":true,
    "screenshot_url":"http://software-center.ubuntu.com/site_media/appmedia/2012/09/Screen_03.jpg",
    "terms_of_service":"Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.",
    "name":"org.example.fantastiqueapp",
    "title":"The Fantastic Application",
    "support_url":"http://support.example.org/fantasticapp",
    "icon_url":"http://software-center.ubuntu.com/site_media/appmedia/2012/09/SPAZ.png",
    "binary_filesize":123456,
    "version":"1.2.3",
    "company_name":"Fantastic Company",
    "website":"http://example.org/fantasticapp/",
    "description":"This application does magic.",
    "price":9.99,
    "date_published":"2013-06-01T01:23:45.678Z",
    "license":"AGPL3",
    "click_updown_url": "https://jenkins.qa.ubuntu.com/job/dropping-letters-click/4/artifact/com.ubuntu.dropping-letters_0.1.2.2_all.click",
    "click_version":"0.1",
    "license_key_path":"config/license.txt",
    "screenshot_urls":[
       "http://software-center.ubuntu.com/site_media/appmedia/2012/09/Screen_01.jpg",
       "http://software-center.ubuntu.com/site_media/appmedia/2012/09/Screen_02.jpg",
       "http://software-center.ubuntu.com/site_media/appmedia/2012/09/Screen_03.jpg"],
    "keywords":[
       "fantastic",
       "awesome",
       "app"],
    "video_urls":["http://vimeo.com/61594280"],
    "click_framework":["ubuntu-sdk-13.10"]
}
""";
*/

const string FAKE_JSON_PACKAGE_DETAILS = """
[{"requires_license_key": false, "video_urls": ["http://vimeo.com/61594280"], "website_en": "http://example.org/full_app/en/", "description_fr": "Cette application contient des valeurs pour tous les champs dans le sch\u00e9ma de Solr.", "screenshot_url": "http://software-center.ubuntu.com/site_media/appmedia/2012/09/Screen_03.jpg", "title_fr": "L'application la plus compl\u00e8te", "icon_url": "http://software-center.ubuntu.com/site_media/appmedia/2012/09/SPAZ.png", "binary_filesize": 123456, "download_url": "http://software-center.ubuntu.com/org.example.full_app/full_app-0.1.1.tar.gz", "version": "0.1.1", "company_name": "The Fantastic App Company", "screenshot_urls": ["http://software-center.ubuntu.com/site_media/appmedia/2012/09/Screen_01.jpg", "http://software-center.ubuntu.com/site_media/appmedia/2012/09/Screen_02.jpg", "http://software-center.ubuntu.com/site_media/appmedia/2012/09/Screen_03.jpg"], "support_url_en": "http://example.org/full_app/support/en/", "title_en": "The Full Application", "price": 0.0, "terms_of_service_en": "http://example.org/full_app/terms/en/", "description_en": "This application does magic.", "framework": ["ubuntu-sdk-13.10"], "icon_urls": ["64|http://software-center.ubuntu.com/site_media/appmedia/2012/09/SPAZ.png"], "keywords_en": ["fantastic app", "fantastic application", "absolutely fantastic", "app", "searchterm"], "name": "org.example.full_app_real_icons", "license": "GPLv3", "date_published": "2013-01-01 00:00:00", "click_version": "0.1", "license_key_path": "", "keywords_fr": ["app fantastique", "application fantastique", "fantastique", "app"]}]
""";
