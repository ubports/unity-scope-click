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

import logging
import os

from unityclickscope import fixture_setup

from autopilot.matchers import Eventually
from testtools.matchers import Equals
from unity8.shell import tests as unity_tests


logger = logging.getLogger(__name__)


class BaseClickScopeTestCase(unity_tests.UnityTestCase):

    scenarios = [
        ('Desktop Nexus 4', dict(
            app_width=768, app_height=1280, grid_unit_px=18)),
        ('Desktop Nexus 10',
            dict(app_width=2560, app_height=1600, grid_unit_px=20))
    ]

    def setUp(self):
        super(BaseClickScopeTestCase, self).setUp()
        if os.environ.get('U1_SEARCH_BASE_URL') == 'fake':
            self.useFixture(fixture_setup.FakeSearchServerRunning())
            self._restart_scope()
        self.launch_unity()
        self._unlock_screen()
        self.dash = self.main_window.get_dash()
        self.scope = self.dash.get_scope('applications')

    def _restart_scope(self):
        logging.info('Restarting click scope.')
        os.system('pkill click-scope')
        os.system(
            "dpkg-architecture -c "
            "'/usr/lib/$DEB_HOST_MULTIARCH//unity-scope-click/click-scope' &")

    def _unlock_screen(self):
        self.main_window.get_greeter().swipe()

    def _open_scope_scrolling(self):
        # TODO move this to the unity8 dash emulator. --elopio - 2013-12-27
        start_x = self.dash.width / 3 * 2
        end_x = self.dash.width / 3
        start_y = end_y = self.dash.height / 2
        self.touch.drag(start_x, start_y, end_x, end_y)


class TestCaseWithHomeScopeOpen(BaseClickScopeTestCase):

    def test_open_scope_scrolling(self):
        self.assertFalse(self.scope.isCurrent)
        self._open_scope_scrolling()
        self.assertThat(self.scope.isCurrent, Eventually(Equals(True)))


class TestCaseWithClickScopeOpen(BaseClickScopeTestCase):

    def setUp(self):
        super(TestCaseWithClickScopeOpen, self).setUp()
        self._open_scope()

    def _open_scope(self):
        self._open_scope_scrolling()
        self.scope.isCurrent.wait_for(True)

    def test_search_available_app(self):
        self._search('Shorts')
        self.scope.wait_select_single('Tile', text='Shorts')

    def _search(self, query):
        # TODO move this to the unity8 main view emulator.
        # --elopio - 2013-12-27
        search_box = self._proxy.select_single("SearchIndicator")
        self.touch.tap_object(search_box)
        self.keyboard.type(query)
