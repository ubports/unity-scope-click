# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
#
# Copyright (C) 2013, 2014 Canonical Ltd.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3, as published
# by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

import copy
import http.server
import json
import logging
import os
import tempfile
import urllib.parse

import autopilot.logging


logger = logging.getLogger(__name__)


class BaseFakeHTTPRequestHandler(http.server.BaseHTTPRequestHandler):

    def send_json_reply(self, code, reply_json):
        self.send_response(code)
        self.send_header('Content-Type', 'application/json')
        self.end_headers()
        self.wfile.write(reply_json.encode())

    def send_file(self, file_path, send_body=True, extra_headers={}):
        with open(file_path) as file_:
            data = file_.read()
            self.send_response(200)
            self.send_header('Content-Length', str(len(data)))
            for key, value in extra_headers.items():
                self.send_header(key, value)
            self.end_headers()
            if send_body:
                self.wfile.write(data)

    def send_file_headers(self, file_path, extra_headers={}):
        self.send_file(file_path, send_body=False, extra_headers=extra_headers)


class FakeSearchServer(http.server.HTTPServer, object):

    def __init__(self, server_address):
        super(FakeSearchServer, self).__init__(
            server_address, FakeSearchRequestHandler)


class FakeSearchRequestHandler(BaseFakeHTTPRequestHandler):

    _SEARCH_PATH = '/api/v1/search'
    _FAKE_DELTA_RESULTS = {
        "publisher": "Rodney Dawes",
        "_links": {
            "self": {
                "href": (
                    "{U1_SEARCH_BASE_URL}api/v1/"
                    "package/com.ubuntu.developer.dobey.delta-web")
            }
        },
        "architecture": ["all"],
        "title": "Delta",
        "icon_url": "http://TODO/delta-web.png",
        "price": 0.0,
        "name": "com.ubuntu.developer.dobey.delta-web"
    }
    _FAKE_SEARCH_RESPONSE_DICT = {
        "_embedded": {
            "clickindex:package": [_FAKE_DELTA_RESULTS]
        },
    }
    _FAKE_DELTA_DETAILS_DICT = {
        "website": "",
        "description": (
            "A simple web app for Delta.\n"
            "Check in, view flight schedules, and book flights, on the Delta "
            "mobile web site."),
        "price": 0.0,
        "date_published": "2014-05-03T15:30:16.431511Z",
        "framework": ["ubuntu-sdk-14.04-qml-dev1"],
        "terms_of_service": "",
        "prices": {"USD": 0.0},
        "screenshot_url": "http://TODO/delta-web-checkin.png",
        "category": "Utility",
        "publisher": "Rodney Dawes",
        "name": "com.ubuntu.developer.dobey.delta-web",
        "license": "GNU GPL v3",
        "title": "Delta",
        "support_url": "https://launchpad.net/~dobey",
        "icon_url": "http://TODO/delta-web.png",
        "changelog": "",
        "binary_filesize": 23728,
        "download_url": (
            '{DOWNLOAD_BASE_URL}download/delta-dummy.click'),
        "click_version": "0.1",
        "developer_name": "Rodney Dawes",
        "version": "1.0.1",
        "company_name": "",
        "keywords": [
            "delta",
            "airlines",
            "flight",
            "status",
            "schedules"
        ],
        "department": ["Accessories"],
        "screenshot_urls": [
            "http://TODO/delta-web-checkin.png",
            "https://TODO/delta-web-main.png"
        ],
        "architecture": ["all"]
    }
    _FAKE_DETAILS = {
        'com.ubuntu.developer.dobey.delta-web': _FAKE_DELTA_DETAILS_DICT
    }
    _FAKE_INDEX = {
        "_embedded": {
            "clickindex:department": [
                {
                    "has_children": False,
                    "_links": {
                        "self": {
                            "href": (
                                "{U1_SEARCH_BASE_URL}api/v1/departments/"
                                "accessories")
                        }
                    },
                    "name": "Accessories",
                    "slug": "accesories"
                },
            ],
            "clickindex:highlight": [
                {
                    "_embedded": {
                        "clickindex:package": [_FAKE_DELTA_RESULTS],
                    },
                    "_links": {
                        "self": {
                            "href": (
                                "{U1_SEARCH_BASE_URL}api/v1/highlights/"
                                "travel-apps")
                        }
                    },
                    "name": "Travel apps",
                    "slug": "travel-apps"
                },
            ],
        },
        '_links': {
            'clickindex:department': {
                'href': "{U1_SEARCH_BASE_URL}api/v1/departments/{slug}",
                'templated': True,
                'title': 'Department'
            },
            'clickindex:departments': {
                'href': "{U1_SEARCH_BASE_URL}api/v1/departments",
                'title': 'Departments'
            },
            'clickindex:highlight': {
                'href': '{U1_SEARCH_BASE_URL}api/v1/highlights/{slug}',
                'templated': True,
                'title': 'Highlight'
            },
            'clickindex:highlights': {
                'href': '{U1_SEARCH_BASE_URL}api/v1/highlights',
                'title': 'Highlights'
            },
            'clickindex:package': {
                'href': '{U1_SEARCH_BASE_URL}api/v1/package/{name}',
                'templated': True,
                'title': 'Package'
            },
            'curies': [
                {
                    'href': '{U1_SEARCH_BASE_URL}docs/v1/relations.html{#rel}',
                    'name': 'clickindex',
                    'templated': True
                }
            ],
            'search': {
                'href': '{U1_SEARCH_BASE_URL}api/v1/search{?q}',
                'templated': True,
                'title': 'Search'
            },
            'self': {
                'href': '{U1_SEARCH_BASE_URL}api/v1'
            }
        }
    }

    def do_GET(self):
        parsed_path = urllib.parse.urlparse(self.path)
        if parsed_path.path.startswith(self._SEARCH_PATH):
            self.send_search_results()
        elif parsed_path.path.startswith('/extra/'):
            self.send_file(parsed_path.path[1:])
        elif parsed_path.path.startswith('/api/v1/package/'):
            package = parsed_path.path[16:]
            self.send_package_details(package)
        elif parsed_path.path.startswith('/api/v1'):
            self.send_index()
        else:
            logger.error(
                'Not implemented path in fake server: {}'.format(self.path))
            raise NotImplementedError(self.path)

    @autopilot.logging.log_action(logger.debug)
    def send_search_results(self):
        results = json.dumps(self._FAKE_SEARCH_RESPONSE_DICT)
        self.send_json_reply(200, results)

    @autopilot.logging.log_action(logger.debug)
    def send_package_details(self, package):
        details = copy.deepcopy(self._FAKE_DETAILS.get(package, None))
        if details is not None:
            details['download_url'] = details['download_url'].format(
                DOWNLOAD_BASE_URL=os.environ.get('DOWNLOAD_BASE_URL'))
            self.send_json_reply(
                200, json.dumps(details))
        else:
            raise NotImplementedError(package)

    @autopilot.logging.log_action(logger.debug)
    def send_index(self):
        self.send_json_reply(200, json.dumps(self._FAKE_INDEX))


class FakeDownloadServer(http.server.HTTPServer, object):

    def __init__(self, server_address):
        super(FakeDownloadServer, self).__init__(
            server_address, FakeDownloadRequestHandler)


class FakeDownloadRequestHandler(BaseFakeHTTPRequestHandler):

    def do_HEAD(self):
        parsed_path = urllib.parse.urlparse(self.path)
        if parsed_path.path.startswith('/download/'):
            self._send_dummy_file_headers(parsed_path.path[10:])
        else:
            raise NotImplementedError(self.path)

    def _send_dummy_file_headers(self, name):
        dummy_file_path = self._make_dummy_file(name)
        self.send_file_headers(
            dummy_file_path, extra_headers={'X-Click-Token': 'dummy'})
        os.remove(dummy_file_path)

    def _make_dummy_file(self, name):
        dummy_file = tempfile.NamedTemporaryFile(
            prefix='dummy', suffix='.click', delete=False)
        dummy_file.write('Dummy click file.'.encode())
        dummy_file.write(name.encode())
        dummy_file.close()
        return dummy_file.name
