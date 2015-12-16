# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
#
# Copyright (C) 2015 Canonical Ltd.
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

import fixtures
import http.server
import logging
import os
import socketserver

from fake_server_fixture import FakeServerFixture
from scope_harness.testing import ScopeHarnessTestCase

logger = logging.getLogger(__name__)


class FakeSearchRequestHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        self.send_content(head_only=False)

    def do_HEAD(self):
        self.send_content(head_only=True)

    def send_content(self, head_only=False):
        path = self.translate_path(self.server.root_folder + self.path)
        logger.info("opening this path: %s", path)
        logger.info("server serves at: %s", self.server.root_folder)
        if self.server.needs_auth:
            if "Authorization" not in self.headers:
                self.send_error(401, "Not authorized")
                return
        if os.path.isdir(path):
            path = os.path.join(path, "index.json")
        try:
            f = open(path, "rb")
            contents = f.read()
            new_url = self.server.url
            replaced_contents = contents.replace(b"[FAKE_SERVER_BASE]",
                                                 new_url.encode("utf-8"))
        except OSError:
            self.send_error(404, "File not found")
            return

        try:
            self.send_response(200)
            self.send_header("Content-type", "application/json")
            self.send_header("Content-Length", len(replaced_contents))
            self.end_headers()

            if (head_only):
                return

            self.wfile.write(replaced_contents)

        finally:
            f.close()


class FakeServer(socketserver.ForkingMixIn, http.server.HTTPServer):
    def __init__(self, server_address, root_folder, needs_auth):
        super().__init__(server_address, FakeSearchRequestHandler)
        self.root_folder = root_folder
        self.needs_auth = needs_auth


class ScopeTestBase(ScopeHarnessTestCase, fixtures.TestWithFixtures):

    def setupJsonServer(self, env_var, root_folder, append_slash=False,
            needs_auth=False):
        server_fixture = FakeServerFixture(FakeServer, root_folder,
            needs_auth)
        self.useFixture(server_fixture)
        self.useFixture(fixtures.EnvironmentVariable(
            env_var,
            newvalue=server_fixture.url))

    def setUp(self):
        self.useFixture(fixtures.TempHomeDir())
        self.useFixture(fixtures.EnvironmentVariable(
            'LANGUAGE',
            newvalue='en_US.utf-8'))

        if os.environ.get('U1_SEARCH_BASE_URL', 'fake') == 'fake':
            self.setupJsonServer("U1_SEARCH_BASE_URL",
                                 "fake_responses/click-package-index/",
                                 append_slash=True,
                                 needs_auth=True)
        if os.environ.get('U1_REVIEWS_BASE_URL', 'fake') == 'fake':
            self.setupJsonServer("U1_REVIEWS_BASE_URL",
                                 "fake_responses/ratings-and-reviews/")
        if os.environ.get('PAY_BASE_URL', 'fake') == 'fake':
            self.setupJsonServer("PAY_BASE_URL",
                                 "fake_responses/software-center-agent/",
                                 needs_auth=True)
