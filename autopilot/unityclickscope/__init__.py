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
        self._swipe_to_bottom()
        # TODO call click_scope_item once the fix for bug http://pad.lv/1335548
        # lands. --elopio - 2014-06-28
        category_element = self._get_category_element('store')
        icon = category_element.select_single(
            'AbstractButton', title='Ubuntu Store')
        self.pointing_device.click_object(icon)
        store_scope = self.get_root_instance().select_single(
            'GenericScopeView', objectName='dashTempScopeItem')
        store_scope.isCurrent.wait_for(True)

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
            # one custom proxy object to match.
            if state['objectName'][1] == 'dashTempScopeItem':
                return True
        return False

    @autopilot.logging.log_action(logger.info)
    def open_preview(self, category, app_name):
        """Open the preview of an application.

        :parameter category: The name of the category where the application is.
        :parameter app_name: The name of the application.
        :return: The opened preview.

        """
        category_element = self._get_category_element(category)
        icon = category_element.select_single('AbstractButton', title=app_name)
        self.pointing_device.click_object(icon)
        # TODO assign an object name to this preview list view.
        # --elopio - 2014-06-29
        preview_list = self._get_scope_item().wait_select_single(
            'PreviewListView', objectName='')
        preview_list.x.wait_for(0)
        return preview_list.select_single(
            Preview, objectName='preview{}'.format(preview_list.currentIndex))


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
        parent = self.get_parent()
        install_button = self.select_single(
            'PreviewActionButton', objectName='buttoninstall_click')
        self.pointing_device.click_object(install_button)
        self.implicitHeight.wait_for(0)
        parent.ready.wait_for(True)

    def is_progress_bar_visible(self):
        try:
            self.select_single('ProgressBar', objectName='progressBar')
            return True
        except exceptions.StateNotFoundError:
            return False
