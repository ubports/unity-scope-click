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
import subprocess
import os

import dbusmock
import fixtures
from autopilot.introspection import dbus as autopilot_dbus
from autopilot.matchers import Eventually
from testtools.matchers import Equals
from ubuntuuitoolkit import emulators as toolkit_emulators
from unity8 import process_helpers
from unity8.shell import (
    emulators as unity_emulators,
    tests as unity_tests
)

from unityclickscope import credentials, fake_services, fixture_setup


logger = logging.getLogger(__name__)


class BaseClickScopeTestCase(dbusmock.DBusTestCase, unity_tests.UnityTestCase):

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
        if os.environ.get('DOWNLOAD_BASE_URL') == 'fake':
            self._use_fake_download_server()
            self._use_fake_download_service()

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

    def _use_fake_download_server(self):
        fake_download_server = fixture_setup.FakeDownloadServerRunning()
        self.useFixture(fake_download_server)
        self.useFixture(fixtures.EnvironmentVariable(
            'DOWNLOAD_BASE_URL', newvalue=fake_download_server.url))

    def _use_fake_download_service(self):
        self._spawn_fake_downloader()

        dbus_connection = self.get_dbus(system_bus=False)
        fake_download_service = fake_services.FakeDownloadService(
            dbus_connection)

        fake_download_service.add_method(
            'getAllDownloadsWithMetadata', return_value=[])

        download_object_path = '/com/canonical/applications/download/test'
        fake_download_service.add_method(
            'createDownload', return_value=download_object_path)

        fake_download_service.add_download_object(download_object_path)

    def _spawn_fake_downloader(self):
        download_manager_mock = self.spawn_server(
            'com.canonical.applications.Downloader',
            '/',
            'com.canonical.applications.DownloadManager',
            system_bus=False,
            stdout=subprocess.PIPE)
        self.addCleanup(self._terminate_process, download_manager_mock)

    def _terminate_process(self, dbus_mock):
        dbus_mock.terminate()
        dbus_mock.wait()

    def _restart_scope(self):
        logging.info('Restarting click scope.')
        os.system('pkill click-scope')
        os.system(
            "dpkg-architecture -c "
            "'/usr/lib/$DEB_HOST_MULTIARCH//unity-scope-click/click-scope' &")

    def _unlock_screen(self):
        self.main_window.get_greeter().swipe()

    def open_scope(self):
        self.dash.open_scope('applications')
        self.scope.isCurrent.wait_for(True)

    def open_app_preview(self, name):
        self.search(name)
        icon = self.scope.wait_select_single('Tile', text=name)
        pointing_device = toolkit_emulators.get_pointing_device()
        pointing_device.click_object(icon)
        preview = self.dash.wait_select_single(AppPreview)
        preview.showProcessingAction.wait_for(False)
        return preview

    def search(self, query):
        # TODO move this to the unity8 main view emulator.
        # --elopio - 2013-12-27
        search_box = self._proxy.select_single("SearchIndicator")
        self.touch.tap_object(search_box)
        self.keyboard.type(query)


class TestCaseWithHomeScopeOpen(BaseClickScopeTestCase):

    def test_open_scope_scrolling(self):
        self.assertFalse(self.scope.isCurrent)
        self.dash.open_scope('applications')
        self.assertThat(self.scope.isCurrent, Eventually(Equals(True)))


class TestCaseWithClickScopeOpen(BaseClickScopeTestCase):

    def setUp(self):
        super(TestCaseWithClickScopeOpen, self).setUp()
        self.open_scope()

    def test_search_available_app(self):
        self.search('Shorts')
        self.scope.wait_select_single('Tile', text='Shorts')

    def test_open_app_preview(self):
        expected_details = dict(
            title='Shorts', publisher='Ubuntu Click Loader')
        preview = self.open_app_preview('Shorts')
        details = preview.get_details()
        self.assertEqual(details, expected_details)

    def test_install_without_credentials(self):
        preview = self.open_app_preview('Shorts')
        preview.install()
        error = self.dash.wait_select_single(DashPreview)
        details = error.get_details()
        self.assertEqual('Login Error', details.get('title'))


class ClickScopeTestCaseWithCredentials(BaseClickScopeTestCase):

    def setUp(self):
        self.add_u1_credentials()
        super(ClickScopeTestCaseWithCredentials, self).setUp()
        self.open_scope()
        self.preview = self.open_app_preview('Shorts')

    def add_u1_credentials(self):
        account_manager = credentials.AccountManager()
        account = account_manager.add_u1_credentials(
            'dummy@example.com', 'dummy')
        self.addCleanup(account_manager.delete_account, account)

    def test_install_with_credentials_must_start_download(self):
        self.assertFalse(self.preview.is_progress_bar_visible())

        self.preview.install()
        self.assertThat(
            self.preview.is_progress_bar_visible, Eventually(Equals(True)))


class Preview(object):

    def get_details(self):
        """Return the details of the open preview."""
        title = self.select_single('Label', objectName='titleLabel').text
        subtitle = self.select_single(
            'Label', objectName='subtitleLabel').text
        # The description label doesn't have an object name. Reported as bug
        # http://pad.lv/1269114 -- elopio - 2014-1-14
        # description = self.select_single(
        #    'Label', objectName='descriptionLabel').text
        # TODO check screenshots, icon, rating and reviews.
        return dict(title=title, subtitle=subtitle)


# TODO move this to unity. --elopio - 2014-1-22
class DashPreview(unity_emulators.UnityEmulatorBase, Preview):
    """Autopilot emulator for the generic preview."""


# TODO move this to unity. --elopio - 2014-1-14
class AppPreview(unity_emulators.UnityEmulatorBase, Preview):
    """Autopilot emulator for the application preview."""

    def get_details(self):
        """Return the details of the application whose preview is open."""
        details = super(AppPreview, self).get_details()
        return dict(
            title=details.get('title'), publisher=details.get('subtitle'))

    def install(self):
        install_button = self.select_single('Button', objectName='button0')
        if install_button.text != 'Install':
            raise unity_emulators.UnityEmulatorException(
                'Install button not found.')
        self.pointing_device.click_object(install_button)
        self.implicitHeight.wait_for(0)

    def is_progress_bar_visible(self):
        try:
            self.select_single('ProgressBar', objectName='progressBar')
            return True
        except autopilot_dbus.StateNotFoundError:
            return False
