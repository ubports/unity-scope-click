/*
 * Copyright (C) 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#ifndef FAKE_JSON_H
#define FAKE_JSON_H

const std::string FAKE_JSON_REVIEWS_RESULT_ONE = R"foo(
        [
          {
            "date_created": "2014-01-28T09:09:47.218Z",
            "date_deleted": null,
            "hide": false,
            "id": 1,
            "language": "en",
            "package_name": "com.example.fakepackage",
            "rating": 4,
            "review_text": "It is ok.",
            "reviewer_displayname": "Reviewer",
            "reviewer_username": "reviewer",
            "summary": "Review Summary",
            "usefulness_favorable": 0,
            "usefulness_total": 0,
            "version": "0.2"
          }
        ]
)foo";

const std::string FAKE_JSON_SEARCH_RESULT_ONE = R"foo({
        "_embedded": {
            "clickindex:package": [
                {
                    "name": "org.example.awesomelauncher",
                    "title": "Awesome Launcher",
                    "description": "This is an awesome launcher.",
                    "price": 1.99,
                    "icon_url": "http://software-center.ubuntu.com/site_media/appmedia/2012/09/SPAZ.png",
                    "resource_url": "http://search.apps.ubuntu.com/api/v1/package/org.example.awesomelauncher"
                }
            ]
        }
    }
)foo";

const std::string FAKE_JSON_SEARCH_RESULT_MANY = R"foo({
        "_embedded": {
            "clickindex:package": [
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
        }
    }
)foo";

const std::string FAKE_JSON_PACKAGE_DETAILS = R"foo(
    {
        "name": "ar.com.beuno.wheather-touch",
        "icon_url": "http://developer.staging.ubuntu.com/site_media/appmedia/2013/07/weather-icone-6797-64.png",
        "title": "\u1F4A9 Weather",
        "description": "\u1F4A9 Weather\nA weather application.",
        "download_url": "https://public.apps.staging.ubuntu.com/download/ar.com.beuno/wheather-touch/ar.com.beuno.wheather-touch-0.2",
        "rating": 3.5,
        "keywords": "these, are, key, words",
        "terms_of_service": "tos",
        "license": "Proprietary",
        "publisher": "Beuno",
        "screenshot_url": "sshot0",
        "screenshot_urls": ["sshot1", "sshot2"],
        "binary_filesize": 177582,
        "version": "0.2",
        "framework": "None",

        "website": "",
        "support_url": "http://beuno.com.ar",
        "price": 1.99,
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

const std::string FAKE_JSON_DEPARTMENTS_ONLY = R"(
  {
    "_links": {
        "self": {
            "href": "https://search.apps.ubuntu.com/api/v1/departments"
        },
        "curies": [
            {
                "name": "clickindex",
                "href": "https://search.apps.ubuntu.com/docs/v1/relations.html{#rel}",
                "templated": true
            }
        ]
    },
    "_embedded": {
        "clickindex:department": [
            {
                "name": "Games",
                "_links": {
                    "self": {
                        "href": "https://search.apps.ubuntu.com/api/v1/departments/Games"
                    }
                },
                "_embedded": {
                    "clickindex:department": [
                        {
                            "name": "Board Games",
                            "_links": {
                                "self": {
                                    "href": "https://search.apps.ubuntu.com/api/v1/departments/Games/Board+Games"
                                }
                            }
                        }
                    ]
                }
            },
            {
                "name": "Graphics",
                "_links": {
                    "self": {
                        "href": "https://search.apps.ubuntu.com/api/v1/departments/Graphics"
                    }
                },
                "_embedded": {
                    "clickindex:department": [
                        {
                            "name": "Drawing",
                            "_links": {
                                "self": {
                                    "href": "https://search.apps.ubuntu.com/api/v1/departments/Graphics/Drawing"
                                }
                            }
                        }
                    ]
                }
            },
            {
                "name": "Internet",
                "_links": {
                    "self": {
                        "href": "https://search.apps.ubuntu.com/api/v1/departments/Internet"
                    }
                },
                "_embedded": {
                    "clickindex:department": [
                        {
                            "name": "Chat",
                            "_links": {
                                "self": {
                                    "href": "https://search.apps.ubuntu.com/api/v1/departments/Internet/Chat"
                                }
                            }
                        },
                        {
                            "name": "Mail",
                            "_links": {
                                "self": {
                                    "href": "https://search.apps.ubuntu.com/api/v1/departments/Internet/Mail"
                                }
                            }
                        },
                        {
                            "name": "Web Browsers",
                            "_links": {
                                "self": {
                                    "href": "https://search.apps.ubuntu.com/api/v1/departments/Internet/Web+Browsers"
                                }
                            }
                        }
                    ]
                }
            }
        ]
    }
})";

#endif // FAKE_JSON_H
