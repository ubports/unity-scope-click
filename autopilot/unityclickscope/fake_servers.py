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

import BaseHTTPServer
import copy
import json
import os
import tempfile
import urlparse


class BaseFakeHTTPRequestHandler(BaseHTTPServer.BaseHTTPRequestHandler):

    def send_json_reply(self, code, reply_json):
        self.send_response(code)
        self.send_header('Content-Type', 'application/json')
        self.end_headers()
        self.wfile.write(reply_json)

    def send_file(self, file_path, send_body=True, extra_headers={}):
        with open(file_path) as file_:
            data = file_.read()
            self.send_response(200)
            self.send_header('Content-Length', str(len(data)))
            for key, value in extra_headers.iteritems():
                self.send_header(key, value)
            self.end_headers()
            if send_body:
                self.wfile.write(data)

    def send_file_headers(self, file_path, extra_headers={}):
        self.send_file(file_path, send_body=False, extra_headers=extra_headers)


class FakeSearchServer(BaseHTTPServer.HTTPServer, object):

    def __init__(self, server_address):
        super(FakeSearchServer, self).__init__(
            server_address, FakeSearchRequestHandler)


class FakeSearchRequestHandler(BaseFakeHTTPRequestHandler):

    _SEARCH_PATH = '/api/v1/search'
    _FAKE_SEARCH_RESPONSE_DICT = [
        {
            'resource_url': 'https://TODO/api/v1/package/com.ubuntu.shorts',
            'icon_url': '{U1_SEARCH_BASE_URL}extra/shorts.png',
            'price': 0.0,
            'name': 'com.ubuntu.shorts',
            'title': 'Shorts'
        }
    ]
    _FAKE_SHORTS_DETAILS_DICT = {
        'website': 'https://launchpad.net/ubuntu-rssreader-app',
        'description': (
            'Shorts is an rssreader application\n'
            'Shorts is an rss reader application that allows you to easily '
            'search for new feeds.'),
        'price': 0.0,
        'framework': ["ubuntu-sdk-13.10"],
        'terms_of_service': '',
        'prices': {'USD': 0.0},
        'screenshot_url': 'https://TODO/shorts0.png',
        'date_published': '2013-10-16T15:58:52.469000',
        'publisher': 'Ubuntu Click Loader',
        'name': 'com.ubuntu.shorts',
        'license': 'GNU GPL v3',
        'changelog': 'Test fixes',
        'support_url': 'mailto:ubuntu-touch-coreapps@lists.launchpad.net',
        'icon_url': 'https://TODO/shorts.png',
        'title': 'Shorts',
        'binary_filesize': 164944,
        'download_url': (
            '{DOWNLOAD_BASE_URL}download/shorts-dummy.click'),
        'click_version': '0.1',
        'developer_name': 'Ubuntu Click Loader',
        'version': '0.2.152',
        'company_name': '',
        'keywords': ['shorts', 'rss', 'news'],
        'screenshot_urls': [
            'https://TODO/shorts0.png',
            'https://TODO/shorts1.png'
        ],
        'architecture': ['all']
    }
    _FAKE_DETAILS = {
        'com.ubuntu.shorts': _FAKE_SHORTS_DETAILS_DICT
    }

    def do_GET(self):
        parsed_path = urlparse.urlparse(self.path)
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


class FakeDownloadServer(BaseHTTPServer.HTTPServer, object):

    def __init__(self, server_address):
        super(FakeDownloadServer, self).__init__(
            server_address, FakeDownloadRequestHandler)


class FakeDownloadRequestHandler(BaseFakeHTTPRequestHandler):

    def do_HEAD(self):
        parsed_path = urlparse.urlparse(self.path)
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
        dummy_file.write('Dummy click file.')
        dummy_file.write(name)
        dummy_file.close()
        return dummy_file.name
