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
import urlparse


class FakeSearchServer(BaseHTTPServer.HTTPServer, object):

    def __init__(self, server_address):
        super(FakeSearchServer, self).__init__(
            server_address, FakeSearchRequestHandler)


class FakeSearchRequestHandler(BaseHTTPServer.BaseHTTPRequestHandler):

    _SEARCH_PATH = "/api/v1/search"
    _FAKE_SEARCH_RESPONSE_DICT = [
        {
            'resource_url': 'https://TODO/api/v1/package/com.ubuntu.shorts',
            'icon_url': '{U1_SEARCH_BASE_URL}extra/shorts.png',
            'price': 0.0,
            'name': 'com.ubuntu.shorts',
            'title': 'Shorts'
        }
    ]

    def do_GET(self):
        parsed_path = urlparse.urlparse(self.path)
        if parsed_path.path.startswith(self._SEARCH_PATH):
            self.send_json_reply(200, self._get_fake_search_response())
        elif parsed_path.path.startswith("/extra/"):
            self.send_file(parsed_path.path[1:])
        else:
            raise NotImplementedError(self.path)

    def send_json_reply(self, code, reply_json):
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(reply_json)

    def _get_fake_search_response(self):
        fake_search_response = copy.deepcopy(self._FAKE_SEARCH_RESPONSE_DICT)
        for result in fake_search_response:
            result['icon_url'] = result['icon_url'].format(
                U1_SEARCH_BASE_URL=os.environ.get('U1_SEARCH_BASE_URL'))
        return json.dumps(fake_search_response)

    def send_file(self, file_path):
        with open(file_path) as file_:
            data = file_.read()
            self.send_response(200)
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)
