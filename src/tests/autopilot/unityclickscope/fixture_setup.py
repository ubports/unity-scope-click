# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
#
# Copyright (C) 2013 Canonical Ltd.
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

"""Set up and clean up fixtures for the Unity Click Scope acceptance tests."""

import BaseHTTPServer
import logging
import SocketServer
import threading
import urlparse

import fixtures


_SEARCH_PATH = "/api/v1/search"

logger = logging.getLogger(__name__)


class MultiThreadedHTTPServer(SocketServer.ThreadingMixIn,
                              BaseHTTPServer.HTTPServer):
    pass


class FakeSearchServerRunning(fixtures.Fixture):
    
    def __init__(self):
        super(FakeSearchServerRunning, self).__init__()

    def setUp(self):
        super(FakeSearchServerRunning, self).setUp()
        self._start_fake_server()

    def _start_fake_server(self):
        port = self._get_port()
        logger.info('Starting fake server at port {0}.'.format(port))
        server_address = ('', port)
        fake_server = MultiThreadedHTTPServer(
            server_address, FakeSearchRequestHandler)
        self.addCleanup(self._stop_fake_server, fake_server)
        server_thread = threading.Thread(target=fake_server.serve_forever)
        # Exit the server thread when the main thread terminates.
        server_thread.daemon = True
        server_thread.start()
        self.useFixture(
            fixtures.EnvironmentVariable(
                'U1_SEARCH_BASE_URL',
                newvalue='http://localhost:{0}/'.format(port)))
        
    def _get_port(self):
        # TODO get random free port. The problem then would be, how to pass it
        # to the FakeSearchRequestHandler? --elopio - 2013-12-31.
        return 8888

    def _stop_fake_server(self, server):
        logger.info('Stopping fake server.')
        server.shutdown()


class FakeSearchRequestHandler(BaseHTTPServer.BaseHTTPRequestHandler):

    _FAKE_SEARCH_RESPONSE = """
[
    {
        "resource_url": "https://TODO/api/v1/package/com.ubuntu.shorts",
        "icon_url": "http://localhost:8888/extra/shorts.png",
        "price": 0.0,
        "name": "com.ubuntu.shorts",
        "title": "Shorts"
    }
]
    """

    def do_GET(self):
        parsed_path = urlparse.urlparse(self.path)
        if parsed_path.path.startswith(_SEARCH_PATH):
            self.send_json_reply(200, self._FAKE_SEARCH_RESPONSE)
        elif parsed_path.path.startswith("/extra/"):
            self.send_file(parsed_path.path[1:])
        else:
            raise NotImplementedError(self.path)

    def send_json_reply(self, code, reply_json):
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(reply_json)

    def send_file(self, file_path):
        with open(file_path) as file_:
            data = file_.read()
            self.send_response(200)
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)
