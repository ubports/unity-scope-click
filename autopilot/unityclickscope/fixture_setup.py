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

"""Set up and clean up fixtures for the Unity Click Scope acceptance tests."""


import logging
import threading

import fixtures

from unityclickscope import fake_servers


logger = logging.getLogger(__name__)


class FakeServerRunning(fixtures.Fixture):

    def __init__(self, server_class):
        super(FakeServerRunning, self).__init__()
        self.server_class = server_class

    def setUp(self):
        super(FakeServerRunning, self).setUp()
        self._start_fake_server()

    def _start_fake_server(self):
        logger.info('Starting fake server: {}.'.format(self.server_class))
        server_address = ('', 0)
        fake_server = self.server_class(server_address)
        server_thread = threading.Thread(target=fake_server.serve_forever)
        server_thread.start()
        logger.info('Serving at port {}.'.format(fake_server.server_port))
        self.addCleanup(self._stop_fake_server, server_thread, fake_server)
        self.url = 'http://localhost:{}/'.format(fake_server.server_port)

    def _stop_fake_server(self, thread, server):
        logger.info('Stopping fake server: {}.'.format(self.server_class))
        server.shutdown()
        thread.join()


class FakeSearchServerRunning(FakeServerRunning):

    def __init__(self):
        super(FakeSearchServerRunning, self).__init__(
            fake_servers.FakeSearchServer)


class FakeDownloadServerRunning(FakeServerRunning):

    def __init__(self):
        super(FakeDownloadServerRunning, self).__init__(
            fake_servers.FakeDownloadServer)
