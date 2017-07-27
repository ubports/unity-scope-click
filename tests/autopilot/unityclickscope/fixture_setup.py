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

import fake_server_fixture

from unityclickscope import fake_servers


class FakeSearchServerRunning(fake_server_fixture.FakeServerFixture):

    def __init__(self):
        super(FakeSearchServerRunning, self).__init__(
            fake_servers.FakeSearchServer)


class FakeDownloadServerRunning(fake_server_fixture.FakeServerFixture):

    def __init__(self):
        super(FakeDownloadServerRunning, self).__init__(
            fake_servers.FakeDownloadServer)
