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

from autopilot.matchers import Eventually
from testtools.matchers import Equals
from unity8.shell import tests as unity_tests


class ScopeClickTestCase(unity_tests.UnityTestCase):

    scenarios = [
        ('Desktop Nexus 4', dict(
            app_width=768, app_height=1280, grid_unit_px=18)),
        ('Desktop Nexus 10',
            dict(app_width=2560, app_height=1600, grid_unit_px=20))
    ]

    def setUp(self):
        super(ScopeClickTestCase, self).setUp()
        self.launch_unity()
        self._unlock_screen()
        self.dash = self.main_window.get_dash()
        self.scope = self.dash.get_scope('applications')

    def _unlock_screen(self):
        self.main_window.get_greeter().swipe()

    def test_open_scope_scrolling(self):
        self.assertFalse(self.scope.isCurrent)
        self._open_scope_scrolling()
        self.assertThat(self.scope.isCurrent, Eventually(Equals(True)))

    def _open_scope_scrolling(self):
        # TODO move this to the unity8 dash emulator. --elopio - 2013-12-27
        start_x = self.dash.width / 3 * 2
        end_x = self.dash.width / 3
        start_y = end_y = self.dash.height / 2
        self.touch.drag(start_x, start_y, end_x, end_y)
