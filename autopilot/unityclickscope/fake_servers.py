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
import os
import tempfile
import urllib.parse


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
    _FAKE_SEARCH_RESPONSE_DICT = [
        {
            'resource_url': (
                'https://TODO/api/v1/package/'
                'com.ubuntu.developer.dobey.delta-web'),
            'icon_url': '{U1_SEARCH_BASE_URL}extra/delta.png',
            'price': 0.0,
            'name': 'com.ubuntu.developer.dobey.delta-web',
            'title': 'Delta'
        }
    ]
    _FAKE_DELTA_DETAILS_DICT = {
        'website': '',
        'description': (
            'A simple web app for Delta.\n'
            'Check in, view flight schedules, and book flights, on the Delta '
            'mobile web site.'),
        'price': 0.0,
        'framework': ['ubuntu-sdk-14.04-qml-dev1'],
        'terms_of_service': '',
        'prices': {'USD': 0.0},
        'screenshot_url': 'https://TODO/delta-web-checkin.png',
        'date_published': '2014-05-03T15:30:16.431511Z',
        'publisher': 'Rodney Dawes',
        'name': 'com.ubuntu.developer.dobey.delta-web',
        'license': 'GNU GPL v3',
        'changelog': '',
        'support_url': 'https://launchpad.net/~dobey',
        'icon_url': 'https://TODO/delta-web.png',
        'title': 'Delta',
        'binary_filesize': 23728,
        'download_url': (
            '{DOWNLOAD_BASE_URL}download/delta-dummy.click'),
        'click_version': '0.1',
        'developer_name': 'Rodney Dawes',
        'version': '1.0.1',
        'company_name': '',
        'keywords': ['delta', 'airlines', 'flight', 'status', 'schedules'],
        'screenshot_urls': [
            'https://TODO/delta-web-checkin.png',
            'https://TODO/delta-web-main.png'
        ],
        'architecture': ['all']
    }
    _FAKE_DETAILS = {
        'com.ubuntu.developer.dobey.delta-web': _FAKE_DELTA_DETAILS_DICT
    }

    def do_GET(self):
        parsed_path = urllib.parse.urlparse(self.path)
        if parsed_path.path.startswith(self._SEARCH_PATH):
            self.send_json_reply(200, self._get_fake_search_response())
        elif parsed_path.path.startswith('/extra/'):
            self.send_file(parsed_path.path[1:])
        elif parsed_path.path.startswith('/api/v1/package/'):
            package = parsed_path.path[16:]
            self.send_package_details(package)
        else:
            raise NotImplementedError(self.path)

    def _get_fake_search_response(self):
        fake_search_response = copy.deepcopy(self._FAKE_SEARCH_RESPONSE_DICT)
        for result in fake_search_response:
            result['icon_url'] = result['icon_url'].format(
                U1_SEARCH_BASE_URL=os.environ.get('U1_SEARCH_BASE_URL'))
        return json.dumps(fake_search_response)

    def send_package_details(self, package):
        details = copy.deepcopy(self._FAKE_DETAILS.get(package, None))
        if details is not None:
            details['download_url'] = details['download_url'].format(
                DOWNLOAD_BASE_URL=os.environ.get('DOWNLOAD_BASE_URL'))
            self.send_json_reply(
                200, json.dumps(details))
        else:
            raise NotImplementedError(package)


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
