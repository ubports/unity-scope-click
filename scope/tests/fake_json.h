#ifndef FAKE_JSON_H
#define FAKE_JSON_H

const std::string FAKE_JSON_SEARCH_RESULT_ONE = R"foo(
        [
          {
            "name": "org.example.awesomelauncher",
            "title": "Awesome Launcher",
            "description": "This is an awesome launcher.",
            "price": 1.99,
            "icon_url": "http://software-center.ubuntu.com/site_media/appmedia/2012/09/SPAZ.png",
            "resource_url": "http://search.apps.ubuntu.com/api/v1/package/org.example.awesomelauncher"
          }
        ]
)foo";

const std::string FAKE_JSON_SEARCH_RESULT_MANY = R"foo(
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
)foo";

const std::string FAKE_JSON_PACKAGE_DETAILS = R"foo(
    {
        "name": "ar.com.beuno.wheather-touch",
        "icon_url": "http://developer.staging.ubuntu.com/site_media/appmedia/2013/07/weather-icone-6797-64.png",
        "title": "Weather",
        "description": "Weather\nA weather application.",
        "download_url": "https://public.apps.staging.ubuntu.com/download/ar.com.beuno/wheather-touch/ar.com.beuno.wheather-touch-0.2",
        "rating": 3.5,
        "keywords": "",
        "terms_of_service": "",
        "license": "Proprietary",
        "publisher": "Beuno",
        "screenshot_url": "",
        "screenshot_urls": "",
        "binary_filesize": 177582,
        "version": "0.2",
        "framework": "None",

        "website": "",
        "support_url": "http://beuno.com.ar",
        "price": 0.0,
        "license_key_path": "",
        "click_version": "0.1",
        "company_name": "",
        "icon_urls": {
            "64": "http://developer.staging.ubuntu.com/site_media/appmedia/2013/07/weather-icone-6797-64.png"
        },
        "requires_license_key": false,
        "date_published": "2013-07-16T21:50:34.874000"
    }
)foo";

#endif // FAKE_JSON_H
