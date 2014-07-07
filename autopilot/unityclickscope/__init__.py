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
import ubuntuuitoolkit
from autopilot.introspection import dbus as autopilot_dbus
from testtools.matchers import Equals, MatchesAny
from unity8.shell.emulators import dash


logger = logging.getLogger(__name__)


class GenericScopeView(dash.DashApps):

    """Autopilot helper for the generic scope view of the store."""

    # XXX We need to set an objectName to this scope, so we can put a different
    # name to this custom proxy object class. --elopio - 2014-06-28

    def enter_search_query(self, query):
        # TODO once http://pad.lv/1335551 is fixed, we can use the search
        # helpers from unity. --elopio - 2014-06-28
        search_text_field = self._get_search_text_field()
        search_text_field.write(query)
        search_text_field.state.wait_for('idle')

    def _get_search_text_field(self):
        page_header = self._get_scope_item_header()
        search_container = page_header.select_single(
            'QQuickItem', objectName='searchContainer')
        search_container.state.wait_for(
            MatchesAny(Equals('narrowActive'), Equals('active')))
        return search_container.select_single(ubuntuuitoolkit.TextField)

    def _get_scope_item_header(self):
        return self._get_scope_item().select_single(
            'PageHeader', objectName='')

    def _get_scope_item(self):
        return self.get_root_instance().select_single('ScopeItem')

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


class DashApps(dash.DashApps):

    """Autopilot helper for the applicatios scope."""

    @autopilot.logging.log_action(logger.info)
    def go_to_store(self):
        """Open the applications store.

        :return: The store Scope View.

        """
        # TODO call click_scope_item once the fix for bug http://pad.lv/1335548
        # lands. --elopio - 2014-06-28
        category_element = self._get_category_element('store')
        icon = category_element.select_single(
            'AbstractButton', title='Ubuntu Store')
        self.pointing_device.click_object(icon)
        scope_item = self.get_root_instance().select_single('ScopeItem')
        scope_item.x.wait_for(0)
        return scope_item.select_single(GenericScopeView)


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
        except autopilot_dbus.StateNotFoundError:
            return False
