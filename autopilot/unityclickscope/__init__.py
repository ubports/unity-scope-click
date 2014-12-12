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

import autopilot.logging
from autopilot import exceptions, introspection
from unity8.shell.emulators import dash


logger = logging.getLogger(__name__)


class GenericScopeView(dash.GenericScopeView):

    # XXX workaround for bug http://pad.lv/1350532, which causes the unity8
    # GenericScopeView class to match with the ClickScope and StoreScope.
    # --elopio - 2014-11-28

    @classmethod
    def validate_dbus_object(cls, path, state):
        return False


class ClickScope(GenericScopeView):

    """Autopilot helper for the click scope."""

    @classmethod
    def validate_dbus_object(cls, path, state):
        name = introspection.get_classname_from_path(path)
        if name == b'GenericScopeView':
            if state['objectName'][1] == 'clickscope':
                return True
        return False

    @autopilot.logging.log_action(logger.info)
    def go_to_store(self):
        """Open the applicatmions store.

        :return: The store Scope View.

        """
        # XXX The click_scope_item in unity should take care of swiping if the
        # item is not visible.
        # TODO file a bug. --elopio - 2014-11-28
        self._swipe_to_bottom()
        self.click_scope_item('store', 'Ubuntu Store')
        store_scope = self.get_root_instance().select_single(
            'GenericScopeView', objectName='dashTempScopeItem')
        store_scope.isCurrent.wait_for(True)
        return store_scope

    def _swipe_to_bottom(self):
        list_view = self.select_single(
            'ListViewWithPageHeader', objectName='categoryListView')
        list_view.swipe_to_bottom()


class StoreScope(GenericScopeView):

    """Autopilot helper for the generic scope view of the store."""

    @classmethod
    def validate_dbus_object(cls, path, state):
        name = introspection.get_classname_from_path(path)
        if name == b'GenericScopeView':
            # This is probably not a good objectName. It can cause more than
            # one custom proxy object to match. --elopio - 2014-11-28
            if state['objectName'][1] == 'dashTempScopeItem':
                return True
        return False

    @autopilot.logging.log_action(logger.info)
    def enter_search_query(self, query):
        # XXX the enter_search_query of the dash provided by unity doesn't
        # work for the temp store scope.
        # TODO file a bug. --elopio - 2014-11-28
        search_button = self.select_single(objectName='search_header_button')
        self.pointing_device.click_object(search_button)
        headerContainer = self.select_single(objectName='headerContainer')
        headerContainer.contentY.wait_for(0)
        search_text_field = self.select_single(objectName='searchTextField')
        search_text_field.write(query)
        self.get_root_instance().select_single(
            objectName='processingIndicator').visible.wait_for(False)

    def open_preview(self, category, app_name):
        # XXX the open preview method provided by unity doesn't wait for the
        # preview to be ready.
        # TODO file a bug. --elopio - 2014-11-28
        preview = super(StoreScope, self).open_preview(category, app_name)
        self.get_root_instance().select_single(
            objectName='processingIndicator').visible.wait_for(False)
        return preview


class Preview(dash.Preview):

    """Autopilot helper for the application preview."""

    def get_details(self):
        """Return the details of the application whose preview is open."""
        header_widget = self.select_single('PreviewWidget', objectName='hdr')
        title_label = header_widget.select_single(
            'Label', objectName='titleLabel')
        subtitle_label = header_widget.select_single(
            'Label', objectName='subtitleLabel')
        return dict(
            title=title_label.text, subtitle=subtitle_label.text)

    def install(self):
        install_button = self.select_single(
            'PreviewActionButton', objectName='buttoninstall_click')
        self.pointing_device.click_object(install_button)

    def is_progress_bar_visible(self):
        try:
            self.select_single('ProgressBar', objectName='progressBar')
            return True
        except exceptions.StateNotFoundError:
            return False
