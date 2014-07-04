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
import shutil
import subprocess

import dbusmock
import fixtures
from autopilot.matchers import Eventually
from testtools.matchers import Equals
from unity8 import process_helpers
from unity8.shell import tests as unity_tests

import unityclickscope
from unityclickscope import credentials, fake_services, fixture_setup


logger = logging.getLogger(__name__)


class ClickScopeException(Exception):
    """Exception raised when there's a problem with the scope."""


class BaseClickScopeTestCase(dbusmock.DBusTestCase, unity_tests.UnityTestCase):

    scenarios = [
        ('Desktop Nexus 4', dict(
            app_width=768, app_height=1280, grid_unit_px=18)),
        ('Desktop Nexus 10',
            dict(app_width=2560, app_height=1600, grid_unit_px=20))
    ]

    def setUp(self):
        super(BaseClickScopeTestCase, self).setUp()

        # We use fake servers by default because the current Jenkins
        # configuration does not let us override the variables.
        if os.environ.get('DOWNLOAD_BASE_URL', 'fake') == 'fake':
            self._use_fake_download_server()
            self._use_fake_download_service()
        if os.environ.get('U1_SEARCH_BASE_URL', 'fake') == 'fake':
            self._use_fake_server()

        self.useFixture(fixtures.EnvironmentVariable('U1_DEBUG', newvalue='1'))
        self._restart_scopes()

        unity_proxy = self.launch_unity()
        process_helpers.unlock_unity(unity_proxy)
        self.dash = self.main_window.get_dash()
        self.scope = self.dash.get_scope('clickscope')

    def _use_fake_server(self):
        fake_search_server = fixture_setup.FakeSearchServerRunning()
        self.useFixture(fake_search_server)
        self.useFixture(fixtures.EnvironmentVariable(
            'U1_SEARCH_BASE_URL', newvalue=fake_search_server.url))

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

    def _restart_scopes(self):
        logging.info('Restarting click scope.')
        scope_runner_path = self._get_scoperunner_path()
        apps_scope_ini_path, store_scope_ini_path = self._get_scopes_ini_path()

        os.system('pkill -f -9 clickscope.ini')
        os.system('pkill -f -9 clickstore.ini')

        os.system('{scoperunner} "" {appsscope} &'.format(
            scoperunner=scope_runner_path,
            appsscope=apps_scope_ini_path))

        os.system('{scoperunner} "" {storescope} &'.format(
            scoperunner=scope_runner_path,
            storescope=store_scope_ini_path))

    def _get_scoperunner_path(self):
        return os.path.join(
            self._get_installed_unity_scopes_lib_dir(), 'scoperunner')

    def _get_installed_unity_scopes_lib_dir(self):
        arch = subprocess.check_output(
            ["dpkg-architecture", "-qDEB_HOST_MULTIARCH"],
            universal_newlines=True).strip()
        return os.path.join('/usr/lib/{}/'.format(arch), 'unity-scopes')

    def _get_scopes_ini_path(self):
        build_dir = os.environ.get('BUILD_DIR', None)
        if build_dir is not None:
            return self._get_built_scopes_ini_path(build_dir)
        else:
            app_scope_ini_path = os.path.join(
                self._get_installed_unity_scopes_lib_dir(),
                'clickapps', 'clickscope.ini')
            store_scope_ini_path = os.path.join(
                self._get_installed_unity_scopes_lib_dir(),
                'clickstore', 'com.canonical.scopes.clickstore.ini')
            return app_scope_ini_path, store_scope_ini_path

    def _get_built_scopes_ini_path(self, build_dir):
        # The ini and the so files need to be on the same directory.
        # We copy them to a temp directory.
        temp_dir_fixture = fixtures.TempDir()
        self.useFixture(temp_dir_fixture)

        built_apps_scope_ini = os.path.join(
            build_dir, 'data', 'clickscope.ini')
        temp_apps_scope_dir = os.path.join(temp_dir_fixture.path, 'clickapps')
        os.mkdir(temp_apps_scope_dir)
        shutil.copy(built_apps_scope_ini, temp_apps_scope_dir)

        built_apps_scope = os.path.join(
            build_dir, 'scope', 'clickapps', 'scope.so')
        shutil.copy(built_apps_scope, temp_apps_scope_dir)

        built_store_scope_ini = os.path.join(
            build_dir, 'data', 'com.canonical.scopes.clickstore.ini')
        temp_store_scope_dir = os.path.join(
            temp_dir_fixture.path, 'clickstore')
        os.mkdir(temp_store_scope_dir)
        shutil.copy(built_store_scope_ini, temp_store_scope_dir)

        built_store_scope = os.path.join(
            build_dir, 'scope', 'clickstore',
            'com.canonical.scopes.clickstore.so')
        shutil.copy(built_store_scope, temp_store_scope_dir)

        app_scope_ini_path = os.path.join(
            temp_apps_scope_dir, 'clickscope.ini')
        store_scope_ini_path = os.path.join(
            temp_store_scope_dir, 'com.canonical.scopes.clickstore.ini')
        return app_scope_ini_path, store_scope_ini_path

    def _unlock_screen(self):
        self.main_window.get_greeter().swipe()

    def open_scope(self):
        scope = self.dash.open_scope('clickscope')
        scope.isCurrent.wait_for(True)
        return scope

    def search(self, query):
        search_indicator = self._proxy.select_single(
            'SearchIndicator', objectName='search')
        search_indicator.pointing_device.click_object(search_indicator)
        self.scope.enter_search_query(query)

    def open_app_preview(self, category, name):
        self.search(name)
        preview = self.scope.open_preview(category, name)
        preview.get_parent().ready.wait_for(True)
        return preview


class TestCaseWithHomeScopeOpen(BaseClickScopeTestCase):

    def test_open_scope_scrolling(self):
        scope = self.dash.open_scope('clickscope')
        self.assertThat(scope.isCurrent, Equals(True))


class BaseTestCaseWithStoreScopeOpen(BaseClickScopeTestCase):

    def setUp(self):
        super(BaseTestCaseWithStoreScopeOpen, self).setUp()
        app_scope = self.open_scope()
        self.scope = app_scope.go_to_store()


class TestCaseWithStoreScopeOpen(BaseTestCaseWithStoreScopeOpen):

    def test_search_available_app(self):
        self.search('Delta')
        applications = self.scope.get_applications('appstore')
        self.assertThat(applications[0], Equals('Delta'))

    def test_open_app_preview(self):
        expected_details = dict(
            title='Delta', subtitle='Rodney Dawes')
        preview = self.open_app_preview('appstore', 'Delta')
        details = preview.get_details()
        self.assertEqual(details, expected_details)

    def test_install_without_credentials(self):
        preview = self.open_app_preview('appstore', 'Delta')
        preview.install()
        error = self.dash.wait_select_single(unityclickscope.Preview)

        details = error.get_details()
        self.assertEqual('Login Error', details.get('title'))


class ClickScopeTestCaseWithCredentials(BaseTestCaseWithStoreScopeOpen):

    def setUp(self):
        self.add_u1_credentials()
        super(ClickScopeTestCaseWithCredentials, self).setUp()
        self.preview = self.open_app_preview('appstore', 'Delta')

    def add_u1_credentials(self):
        account_manager = credentials.AccountManager()
        account = account_manager.add_u1_credentials(
            'dummy@example.com',
            'name=Ubuntu+One+%40+bollo&'
            'consumer_secret=*********&'
            'token=**************&'
            'consumer_key=*******&'
            'token_secret=************')
        self.addCleanup(account_manager.delete_account, account)

    def test_install_with_credentials_must_start_download(self):
        self.assertFalse(self.preview.is_progress_bar_visible())

        self.preview.install()
        self.assertThat(
            self.preview.is_progress_bar_visible, Eventually(Equals(True)))
