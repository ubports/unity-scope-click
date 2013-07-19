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
        "icon_url": "http://software-center.ubuntu.com/site_media/appmedia/2012/09/SPAZ.png"},
      {
        "id":"2",
        "title":"MyPackage2",
        "price": 0.0,
        "icon_url": "http://assets.ubuntu.com/sites/ubuntu/504/u/img/ubuntu/features/icon-photos-and-videos-64x64.png"},
      {
        "id":"3",
        "title":"MyPackage3",
        "price": 0.0,
        "icon_url": "http://assets.ubuntu.com/sites/ubuntu/504/u/img/ubuntu/features/icon-find-more-apps-64x64.png"}
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
        "screenshot_url":"http://software-center.ubuntu.com/site_media/appmedia/2012/09/Screen_03.jpg",
        "terms_of_service":"Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.",
        "id":"11",
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
        "click_updown_url":"https://jenkins.qa.ubuntu.com/job/dropping-letters-click/4/artifact/com.ubuntu.dropping-letters_0.1.2.2_all.click",
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
        "video_urls":[
          "http://vimeo.com/61594280"],
        "click_framework":["ubuntu-sdk-13.10"]}]
  }
}
""";


