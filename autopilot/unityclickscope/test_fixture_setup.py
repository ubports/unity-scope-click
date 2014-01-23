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

import httplib
import urlparse

import testtools

from unityclickscope import fixture_setup


class FakeSearchServerRunningTestCase(testtools.TestCase):

    def test_fake_search_should_start_and_stop(self):
        fake_search_server = fixture_setup.FakeSearchServerRunning()
        self.addCleanup(self._assert_server_not_running)
        self.useFixture(fake_search_server)
        self.netloc = urlparse.urlparse(fake_search_server.url).netloc
        connection = httplib.HTTPConnection(self.netloc)
        self.addCleanup(connection.close)
        self._do_request(connection)
        self.assertEqual(connection.getresponse().status, 200)

    def _assert_server_not_running(self):
        connection = httplib.HTTPConnection(self.netloc)
        self.assertRaises(Exception, self._do_request, connection)

    def _do_request(self, connection):
        connection.request('GET', '/api/v1/search')
