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


import dbus
import dbusmock


class FakeDownloadService(object):
    """Fake Download Service using a dbusmock interface."""

    def __init__(self, dbus_connection):
        super(FakeDownloadService, self).__init__()
        self.dbus_connection = dbus_connection
        self.mock = self._get_mock_interface()

    def _get_mock_interface(self):
        return dbus.Interface(
            self.dbus_connection.get_object(
                'com.canonical.applications.Downloader', '/'),
            dbusmock.MOCK_IFACE)

    def add_method(self, method, return_value):
        if method == 'getAllDownloadsWithMetadata':
            self._add_getAllDownloadsWithMetadata(return_value)
        elif method == 'createDownload':
            self._add_createDownload(return_value)
        else:
            raise NotImplementedError('Unknown method: {}'.format(method))

    def _add_getAllDownloadsWithMetadata(self, return_value):
        self.mock.AddMethod(
            'com.canonical.applications.DownloadManager',
            'getAllDownloadsWithMetadata',
            'ss', 'ao', 'ret = {}'.format(return_value))

    def _add_createDownload(self, return_value):
        self.mock.AddMethod(
            'com.canonical.applications.DownloadManager',
            'createDownload', '(sssa{sv}a{ss})', 'o',
            'ret = "{}"'.format(return_value))

    def add_download_object(self, object_path):
        self.mock.AddObject(
            object_path,
            'com.canonical.applications.Download',
            {},
            [('start', '', '', '')])
