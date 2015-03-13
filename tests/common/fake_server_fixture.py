# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
#
# Copyright (C) 2013, 2014, 2015 Canonical Ltd.
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

"""Base class for fake server fixtures."""


import logging
import multiprocessing as mp

import fixtures

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)


class FakeServerFixture(fixtures.Fixture):

    def __init__(self, server_class, *server_args):
        super().__init__()
        self.server_class = server_class
        self.server_args = server_args

    def setUp(self):
        super().setUp()
        self._start_fake_server()

    def _server_process(self, queue, server_class):
        logger.info('Starting fake server: {}.'.format(server_class))
        server_address = ('localhost', 0)
        fake_server = server_class(server_address, *self.server_args)
        fake_server.url = 'http://localhost:{}/'.format(fake_server.server_port)
        logger.info('Serving at port {}.'.format(fake_server.server_port))
        queue.put(fake_server.url)
        fake_server.serve_forever()

    def _start_fake_server(self):
        queue = mp.Queue()
        server_process = mp.Process(target=self._server_process,
                                    args=(queue, self.server_class))
        server_process.start()
        self.addCleanup(self._stop_fake_server, server_process)
        self.url = queue.get()

    def _stop_fake_server(self, process):
        logger.info('Stopping fake server: {}.'.format(self.server_class))
        process.terminate()
        process.join()
