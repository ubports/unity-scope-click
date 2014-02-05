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

#endif // FAKE_JSON_H
