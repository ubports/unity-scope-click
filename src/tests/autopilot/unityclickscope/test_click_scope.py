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

import logging
import os

import fixtures
from autopilot.matchers import Eventually
from testtools.matchers import Equals
from ubuntuuitoolkit import emulators as toolkit_emulators
from unity8 import process_helpers
from unity8.shell import (
    emulators as unity_emulators,
    tests as unity_tests
)

from unityclickscope import fixture_setup


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
            self._use_fake_server()
        unity_proxy = self.launch_unity()
        process_helpers.unlock_unity(unity_proxy)
        self.dash = self.main_window.get_dash()
        self.scope = self.dash.get_scope('applications')

    def _use_fake_server(self):
        fake_search_server = fixture_setup.FakeSearchServerRunning()
        self.useFixture(fake_search_server)
        self.useFixture(fixtures.EnvironmentVariable(
            'U1_SEARCH_BASE_URL', newvalue=fake_search_server.url))
        self._restart_scope()

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
        self.dash.open_scope('applications')
        self.assertThat(self.scope.isCurrent, Eventually(Equals(True)))


class TestCaseWithClickScopeOpen(BaseClickScopeTestCase):

    def setUp(self):
        super(TestCaseWithClickScopeOpen, self).setUp()
        self._open_scope()

    def _open_scope(self):
        self.dash.open_scope('applications')
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

    def test_open_app_preview(self):
        expected_details = dict(
            title='Shorts', publisher='Ubuntu Click Loader')
        preview = self._open_app_preview('Shorts')
        details = preview.get_details()
        self.assertEqual(details, expected_details)

    def _open_app_preview(self, name):
        self._search(name)
        icon = self.scope.wait_select_single('Tile', text=name)
        pointing_device = toolkit_emulators.get_pointing_device()
        pointing_device.click_object(icon)
        return self.dash.wait_select_single(AppPreview)


# TODO move this to unity. --elopio - 2014-1-14
class AppPreview(unity_emulators.UnityEmulatorBase):
    """Autopilot emulator for the application preview."""

    def get_details(self):
        """Return the details of the application whose preview is open."""
        title = self.select_single('Label', objectName='titleLabel').text
        publisher = self.select_single(
            'Label', objectName='subtitleLabel').text
        # The description label doesn't have an object name. Reported as bug
        # http://pad.lv/1269114 -- elopio - 2014-1-14
        # description = self.select_single(
        #    'Label', objectName='descriptionLabel').text
        # TODO check screenshots, icon, rating and reviews.
        return dict(title=title, publisher=publisher)
