# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
#
# Copyright (C) 2014 Canonical Ltd.
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

import http.client
import urllib.parse

import testscenarios
import testtools

from unityclickscope import fixture_setup


class FakeServerRunningTestCase(
        testscenarios.TestWithScenarios, testtools.TestCase):

    scenarios = [
        ('fake search server', dict(
            fixture=fixture_setup.FakeSearchServerRunning,
            request_method='GET', request_path='/api/v1/search')),
        ('fake download server', dict(
            fixture=fixture_setup.FakeDownloadServerRunning,
            request_method='HEAD', request_path='/download/dummy.click'))
    ]

    def test_server_should_start_and_stop(self):
        fake_server = self.fixture()
        self.addCleanup(self._assert_server_not_running)
        self.useFixture(fake_server)
        self.netloc = urllib.parse.urlparse(fake_server.url).netloc
        connection = http.client.HTTPConnection(self.netloc)
        self.addCleanup(connection.close)
        self._do_request(connection)
        self.assertEqual(connection.getresponse().status, 200)

    def _assert_server_not_running(self):
        connection = http.client.HTTPConnection(self.netloc)
        self.assertRaises(Exception, self._do_request, connection)

    def _do_request(self, connection):
        connection.request(self.request_method, self.request_path)
