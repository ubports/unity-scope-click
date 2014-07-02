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

#include <string>


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
                    "_links": {
                        "self": {
                            "href": "http://search.apps.ubuntu.com/api/v1/package/org.example.awesomelauncher"
                        }
                    }
                }
            ]
        }
    }
)foo";

const std::string FAKE_JSON_SEARCH_RESULT_MISSING_DATA = R"foo({
        "_embedded": {
            "clickindex:package": [
                {
                    "name": "org.example.awesomelauncher",
                    "title": "Awesome Launcher",
                    "description": "This is an awesome launcher.",
                    "_links": {
                        "self": {
                            "href": "http://search.apps.ubuntu.com/api/v1/package/org.example.awesomelauncher"
                        }
                    }
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
            "_links": {
                "self": {
                    "href": "http://search.apps.ubuntu.com/api/v1/package/org.example.awesomelauncher"
                }
            }
          },
          {
            "name": "org.example.fantastiqueapp",
            "title": "Fantastic App",
            "description": "This is a fantasticc app.",
            "price": 0.0,
            "icon_url": "http://assets.ubuntu.com/sites/ubuntu/504/u/img/ubuntu/features/icon-find-more-apps-64x64.png",
            "_links": {
                "self": {
                    "href": "http://search.apps.ubuntu.com/api/v1/package/org.example.fantasticapp"
                }
            }
          },
          {
            "name": "org.example.awesomewidget",
            "title": "Awesome Widget",
            "description": "This is an awesome widget.",
            "price": 1.99,
            "icon_url": "http://assets.ubuntu.com/sites/ubuntu/504/u/img/ubuntu/features/icon-photos-and-videos-64x64.png",
            "_links": {
                "self": {
                    "href": "http://search.apps.ubuntu.com/api/v1/package/org.example.awesomewidget"
                }
            }
          }
        ]
        }
    }
)foo";

const std::string FAKE_JSON_SEARCH_RESULT_RECOMMENDS = R"foo({
        "_embedded": {
            "clickindex:package": [
                {
                    "name": "org.example.awesomelauncher",
                    "title": "Awesome Launcher",
                    "description": "This is an awesome launcher.",
                    "price": 1.99,
                    "icon_url": "http://software-center.ubuntu.com/site_media/appmedia/2012/09/SPAZ.png",
                    "_links": {
                        "self": {
                            "href": "http://search.apps.ubuntu.com/api/v1/package/org.example.awesomelauncher"
                        }
                    }
                }
            ],
            "clickindex:recommendation": [
                {
                    "name": "org.example.awesomelauncher2",
                    "title": "Awesome Launcher 2",
                    "description": "This is an another awesome launcher.",
                    "price": 1.99,
                    "icon_url": "http://software-center.ubuntu.com/site_media/appmedia/2012/09/SPAZ.png",
                    "_links": {
                        "self": {
                            "href": "http://search.apps.ubuntu.com/api/v1/package/org.example.awesomelauncher2"
                        }
                    }
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

const std::string FAKE_JSON_BOOTSTRAP = R"(
      {
          "_embedded": {
              "clickindex:department": [
                {
                    "has_children": false,
                    "_links": {
                        "self": {
                            "href": "https://search.apps.staging.ubuntu.com/api/v1/departments/fake-subdepartment"}
                    },
                    "name": "Fake Subdepartment", "slug": "fake-subdepartment"}
              ],
              "clickindex:highlight": [
              {
                  "_embedded": {
                      "clickindex:package": [
                       {
                            "publisher": "Awesome Widget Company",
                            "name": "org.example.awesomelauncher",
                            "title": "Awesome Launcher",
                            "price": 1.99,
                            "_links": {
                                "self": {
                                    "href": "https://search.apps.staging.ubuntu.com/api/v1/package/org.example.awesomelauncher"}
                             },
                             "icon": "http://example.org/media/org.example.awesomelauncher/icons/icon16.png"
                        },
                      {
                          "publisher": "Awesome Widget Company",
                          "name": "org.example.awesomewidget",
                          "title": "Awesome Widget", "price": 1.99,
                          "_links": {
                              "self": {
                                  "href": "https://search.apps.staging.ubuntu.com/api/v1/package/org.example.awesomewidget"
                              }
                           },
                          "icon": "http://example.org/media/org.example.awesomewidget/icons/icon16.png"}
                    ]
                  },
                  "_links": {
                      "self": {
                          "href": "https://search.apps.staging.ubuntu.com/api/v1/highlights/top-apps"
                      }
                  },
                  "name": "Top Apps", "slug": "top-apps"
              },
              {
                  "_embedded": {
                      "clickindex:package": [
                      {
                          "publisher": "Awesome Widget Company",
                          "name": "org.example.awesomelauncher",
                          "title": "Awesome Launcher",
                          "price": 1.99,
                          "_links": {
                              "self": {
                                  "href": "https://search.apps.staging.ubuntu.com/api/v1/package/org.example.awesomelauncher"
                              }
                          },
                          "icon": "http://example.org/media/org.example.awesomelauncher/icons/icon16.png"
                      },
                      {
                          "publisher": "Awesome Widget Company",
                          "name": "org.example.awesomewidget",
                          "title": "Awesome Widget",
                          "price": 1.99,
                          "_links": {
                              "self": {
                                  "href": "https://search.apps.staging.ubuntu.com/api/v1/package/org.example.awesomewidget"
                              }
                          },
                          "icon": "http://example.org/media/org.example.awesomewidget/icons/icon16.png"
                      }
                      ]
                  },
                  "_links": {
                      "self": {
                          "href": "https://search.apps.staging.ubuntu.com/api/v1/highlights/most-purchased"
                      }
                  },
                  "name": "Most Purchased",
                  "slug": "most-purchased"
              },
              {
                  "_embedded": {
                      "clickindex:package": [
                      {
                          "publisher": "Awesome Widget Company",
                          "name": "org.example.awesomelauncher",
                          "title": "Awesome Launcher",
                          "price": 1.99,
                          "_links": {
                              "self": {
                                  "href": "https://search.apps.staging.ubuntu.com/api/v1/package/org.example.awesomelauncher"
                              }
                          },
                          "icon": "http://example.org/media/org.example.awesomelauncher/icons/icon16.png"
                      },
                      {
                          "publisher": "Awesome Widget Company",
                          "name": "org.example.awesomewidget",
                          "title": "Awesome Widget",
                          "price": 1.99,
                          "_links": {
                              "self": {
                                  "href": "https://search.apps.staging.ubuntu.com/api/v1/package/org.example.awesomewidget"
                              }
                          },
                          "icon": "http://example.org/media/org.example.awesomewidget/icons/icon16.png"
                      }
                      ]
                  },
                  "_links": {
                      "self": {
                          "href": "https://search.apps.staging.ubuntu.com/api/v1/highlights/new-releases"
                      }
                  },
                  "name": "New Releases",
                  "slug": "new-releases"
              }
            ]
          }, "has_children": true,
              "_links": {
                  "curies": [
                  {
                      "href": "https://search.apps.staging.ubuntu.com/docs/v1/relations.html{#rel}",
                      "name": "clickindex", "templated": true
                  }
                  ],
                      "self": {
                          "href": "https://search.apps.staging.ubuntu.com/api/v1/departments/fake-department-with-subdepartments"
                      },
                      "collection": {
                          "href": "https://search.apps.staging.ubuntu.com/api/v1/departments"
                      }
              },
              "name": "Fake Department With Subdepartments",
              "slug": "fake-department-with-subdepartments"
    })";

const std::string FAKE_JSON_BROKEN_BOOTSTRAP = R"(
      {
          "_embedded": {
              "clickindex:department": [
                {
                    "name": "Broken department"
                }
              ],
              "clickindex:highlight": [
              {
                  "_embedded": {
                      "clickindex:package": [
                       {
                            "publisher": "Awesome Widget Company",
                            "name": "org.example.awesomelauncher",
                            "title": "Awesome Launcher",
                            "price": 1.99,
                            "_links": {
                                "self": {
                                    "href": "https://search.apps.staging.ubuntu.com/api/v1/package/org.example.awesomelauncher"}
                             },
                             "icon": "http://example.org/media/org.example.awesomelauncher/icons/icon16.png"
                        }
                      ]
                  },
                  "_links": {
                      "self": {
                          "href": "https://search.apps.staging.ubuntu.com/api/v1/highlights/top-apps"
                      }
                  },
                  "name": "Top Apps",
                  "slug": "top-apps"
              },
              {
                  "_embedded": {
                      "clickindex:package": [
                      {
                          "publisher": "Awesome Widget Company",
                          "name": "org.example.awesomelauncher",
                          "title": "Awesome Launcher",
                          "price": 1.99,
                          "_links": {
                              "self": {
                                  "href": "https://search.apps.staging.ubuntu.com/api/v1/package/org.example.awesomelauncher"
                              }
                          },
                          "icon": "http://example.org/media/org.example.awesomelauncher/icons/icon16.png"
                      }
                      ]
                  },
                  "____name": "Broken highlight"
              }
            ]
          }
    })";

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
                "slug": "games",
                "_links": {
                    "self": {
                        "href": "https://search.apps.ubuntu.com/api/v1/departments/Games"
                    }
                },
                "_embedded": {
                    "clickindex:department": [
                        {
                            "name": "Board Games",
                            "slug": "board_games",
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
                "slug": "graphics",
                "_links": {
                    "self": {
                        "href": "https://search.apps.ubuntu.com/api/v1/departments/Graphics"
                    }
                },
                "_embedded": {
                    "clickindex:department": [
                        {
                            "name": "Drawing",
                            "slug": "graphics_drawing",
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
                "slug": "internet",
                "_links": {
                    "self": {
                        "href": "https://search.apps.ubuntu.com/api/v1/departments/Internet"
                    }
                },
                "_embedded": {
                    "clickindex:department": [
                        {
                            "name": "Chat",
                            "slug": "internet_chat",
                            "_links": {
                                "self": {
                                    "href": "https://search.apps.ubuntu.com/api/v1/departments/Internet/Chat"
                                }
                            }
                        },
                        {
                            "name": "Mail",
                            "slug": "internet_mail",
                            "_links": {
                                "self": {
                                    "href": "https://search.apps.ubuntu.com/api/v1/departments/Internet/Mail"
                                }
                            }
                        },
                        {
                            "name": "Web Browsers",
                            "slug": "internet_web",
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

const std::string FAKE_JSON_BROKEN_DEPARTMENTS = R"(
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
                "slug": "games",
                "_links": {
                    "self": {
                        "href": "https://search.apps.ubuntu.com/api/v1/departments/Games"
                    }
                },
                "_embedded": {
                    "clickindex:department": [
                        {
                            "name": "Broken department"
                        }
                    ]
                }
            }
        ]
    }
  })";

const std::string FAKE_JSON_MANIFEST_REMOVABLE = R"foo(
    {
        "_removable": 1,
        "name": "com.example.fake",
        "version": "0.1",
        "hooks": {
            "fake-app": {
                "desktop": "fake-app.desktop"
            }
        }
    }
)foo";

const std::string FAKE_JSON_MANIFEST_NONREMOVABLE = R"foo(
    {
        "_removable": 0,
        "name": "com.example.fake",
        "version": "0.1",
        "hooks": {
            "fake-app": {
                "desktop": "fake-app.desktop"
            }
        }
    }
)foo";

const std::string FAKE_JSON_MANIFEST_ONE_APP = R"foo(
    {
        "_removable": 1,
        "name": "com.example.fake-app",
        "version": "0.1",
        "hooks": {
            "fake-app": {
                "apparmor": "fake-app.json",
                "desktop": "fake-app.desktop"
            }
        }
    }
)foo";

const std::string FAKE_JSON_MANIFEST_ONE_SCOPE = R"foo(
    {
        "_removable": 1,
        "name": "com.example.fake-scope",
        "version": "0.1",
        "hooks": {
            "fake-scope": {
                "apparmor": "scope-security.json",
                "scope": "fake-scope"
            }
        }
    }
)foo";

const std::string FAKE_JSON_MANIFEST_ONE_APP_ONE_SCOPE = R"foo(
    {
        "_removable": 1,
        "name": "com.example.fake-1app-1scope",
        "version": "0.1",
        "hooks": {
            "fake-app": {
                "apparmor": "fake-app.json",
                "desktop": "fake-app.desktop"
            },
            "fake-scope": {
                "apparmor": "scope-security.json",
                "scope": "fake-scope"
            }
        }
    }
)foo";

const std::string FAKE_JSON_MANIFEST_TWO_APPS_TWO_SCOPES = R"foo(
    {
        "_removable": 1,
        "name": "com.example.fake-2apps-2scopes",
        "version": "0.1",
        "hooks": {
            "fake-app1": {
                "apparmor": "fake-app1.json",
                "desktop": "fake-app1.desktop"
            },
            "fake-app2": {
                "apparmor": "fake-app2.json",
                "desktop": "fake-app2.desktop"
            },
            "fake-scope1": {
                "apparmor": "scope-security1.json",
                "scope": "fake-scope1"
            },
            "fake-scope2": {
                "apparmor": "scope-security1.json",
                "scope": "fake-scope2"
            }
        }
    }
)foo";

#endif // FAKE_JSON_H
