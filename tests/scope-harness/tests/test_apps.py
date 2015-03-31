# -*- Mode: Python; coding: utf-8; indent-tabs-mode: nil; tab-width: 4 -*-
#
# Copyright (C) 2015 Canonical Ltd.
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

import os
import sys
import testtools

from scope_harness import *
from scope_harness.testing import ScopeHarnessTestCase

class AppsTest (ScopeHarnessTestCase):

    @testtools.skip('No fake local data available.')
    def setUp(self):
        super().setUp()
        scope_dir = os.environ.get('SCOPE_DIR', None)
        self.harness = self.launch_scope(scope_dir)
        self.view = self.harness.results_view
        self.view.active_scope = 'clickscope'

    def launch_scope(self, scope_dir=None):
        """Find the scope and launch it."""
        if scope_dir is None:
            return ScopeHarness.new_from_scope_list(Parameters([
                'clickscope.ini'
            ]))
        else:
            scope_path = os.path.join(
                scope_dir, 'clickapps', 'clickscope.ini')
            return ScopeHarness.new_from_scope_list(Parameters([
                scope_path
            ]))

    def test_surfacing_results(self):
        self.view.browse_department('')
        self.view.search_query = ''

        # Check first apps of every category
        match = CategoryListMatcher() \
            .has_exactly(3) \
            .mode(CategoryListMatcherMode.BY_ID) \
            .category(CategoryMatcher("predefined") \
                    .has_at_least(1) \
                    .mode(CategoryMatcherMode.BY_URI) \
                    .result(ResultMatcher("application:///dialer-app.desktop") \
                    .title('Phone') \
                    .property('installed', True) \
            )) \
            .category(CategoryMatcher("local") \
                      .has_at_least(1) \
                      .mode(CategoryMatcherMode.STARTS_WITH) \
                      .result(ResultMatcher("application:///com.ubuntu.developer.webapps.webapp-amazon_webapp-amazon.*") \
                      .properties({'installed': True, 'version': '1.0.10'}) \
                      .title('Amazon') \
            )) \
            .category(CategoryMatcher("store") \
                      .has_at_least(1) \
                      .mode(CategoryMatcherMode.BY_URI) \
                      .result(ResultMatcher("scope://com.canonical.scopes.clickstore.*") \
            )) \
            .match(self.view.categories)
        self.assertMatchResult(match)

    def test_surfacing_departments(self):
        self.view.search_query = ''

        departments = self.view.browse_department('')

        self.assertTrue(self.view.has_departments)
        self.assertFalse(self.view.has_alt_departments)

        # TODO: list all expected departments (depending on installed apps)
        match = DepartmentMatcher() \
            .mode(DepartmentMatcherMode.STARTS_WITH) \
            .id('') \
            .label('All') \
            .all_label('') \
            .parent_id('') \
            .parent_label('') \
            .is_root(True) \
            .is_hidden(False) \
            .child(ChildDepartmentMatcher('communication')) \
            .child(ChildDepartmentMatcher('games')) \
            .match(departments)
        self.assertMatchResult(match)

    def test_department_browsing(self):
        self.view.search_query = ''

        departments = self.view.browse_department('games')

        match = DepartmentMatcher() \
            .has_exactly(0) \
            .mode(DepartmentMatcherMode.STARTS_WITH) \
            .label('Games') \
            .all_label('') \
            .parent_id('') \
            .parent_label('All') \
            .is_root(False) \
            .is_hidden(False) \
            .match(departments)
        self.assertMatchResult(match)

        # FIXME: scope harness shouldn't report empty categories, so should be exactly 2
        res_match = CategoryListMatcher() \
            .has_at_least(2) \
            .mode(CategoryListMatcherMode.BY_ID) \
            .category(CategoryMatcher("local") \
                      .has_at_least(1) \
                      .mode(CategoryMatcherMode.STARTS_WITH) \
                      .result(ResultMatcher("application:///com.ubuntu.dropping-letters_dropping-letters.*") \
                      .art('/custom/click/.click/users/@all/com.ubuntu.dropping-letters/./dropping-letters.png') \
                      .properties({'installed': True, 'version': '0.1.2.2.67'})
                      .title('Dropping Letters') \
            )) \
            .category(CategoryMatcher("store") \
                      .has_at_least(1) \
                      .mode(CategoryMatcherMode.BY_URI) \
                      .result(ResultMatcher("scope://com.canonical.scopes.clickstore.*dep=games") \
            )) \
            .match(self.view.categories)
        self.assertMatchResult(res_match)

        # browse different department

        departments = self.view.browse_department('communication')
        self.view.search_query = ''

        match = DepartmentMatcher() \
            .has_exactly(0) \
            .mode(DepartmentMatcherMode.STARTS_WITH) \
            .label('Communication') \
            .all_label('') \
            .parent_id('') \
            .parent_label('All') \
            .is_root(False) \
            .is_hidden(False) \
            .match(departments)
        self.assertMatchResult(match)

        # FIXME: scope harness shouldn't report empty categories, so should be exactly 2
        res_match = CategoryListMatcher() \
            .has_at_least(2) \
            .mode(CategoryListMatcherMode.BY_ID) \
            .category(CategoryMatcher("local") \
                      .has_at_least(1) \
                      .mode(CategoryMatcherMode.STARTS_WITH) \
                      .result(ResultMatcher('application:///com.ubuntu.developer.webapps.webapp-amazon_webapp-amazon_1.0.10.desktop')) \
                      .result(ResultMatcher('application:///webbrowser-app.desktop') \
                      .title('Browser') \
            )) \
            .category(CategoryMatcher("store") \
                      .has_at_least(1) \
                      .mode(CategoryMatcherMode.BY_URI) \
                      .result(ResultMatcher("scope://com.canonical.scopes.clickstore.*dep=communication") \
            )) \
            .match(self.view.categories)
        self.assertMatchResult(res_match)

    def test_nonremovable_app_preview(self):
        self.view.browse_department('')
        self.view.search_query = 'Brow'

        pview = self.view.categories[0].results[0].long_press()
        self.assertIsInstance(pview, PreviewView)

        match = PreviewColumnMatcher().column(\
                    PreviewMatcher() \
                        .widget(PreviewWidgetMatcher("hdr")) \
                        .widget(PreviewWidgetMatcher("buttons") \
                                .type("actions") \
                                .data({'actions':[{'id':'open_click', 'label':'Open', 'uri':'application:///webbrowser-app.desktop'}]}) \
                                ) \
                        .widget(PreviewWidgetMatcher("screenshots")) \
                        .widget(PreviewWidgetMatcher("summary")) \
                ).match(pview.widgets)
        self.assertMatchResult(match)

    def test_removable_app_preview(self):
        self.view.browse_department('')
        self.view.search_query = 'Amazon'

        pview = self.view.categories[0].results[0].long_press()
        self.assertIsInstance(pview, PreviewView)

        match = PreviewColumnMatcher().column(\
                    PreviewMatcher() \
                        .widget(PreviewWidgetMatcher("hdr")) \
                        .widget(PreviewWidgetMatcher("buttons") \
                                .type("actions") \
                                .data({'actions':[
                                    {'id':'open_click',
                                     'label':'Open',
                                     'uri':'application:///com.ubuntu.developer.webapps.webapp-amazon_webapp-amazon_1.0.10.desktop'},
                                    {'id':'uninstall_click',
                                     'label':'Uninstall'}]}) \
                                ) \
                        .widget(PreviewWidgetMatcher("summary") \
                                .type('text')) \
                        .widget(PreviewWidgetMatcher("other_metadata") \
                                .type('text')) \
                        .widget(PreviewWidgetMatcher("updates") \
                                .type('text')) \
                        .widget(PreviewWidgetMatcher("whats_new") \
                                .type('text')) \
                        .widget(PreviewWidgetMatcher("rating") \
                                .type('rating-input')) \
                        .widget(PreviewWidgetMatcher("reviews_title") \
                                .type('text')) \
                        .widget(PreviewWidgetMatcher("summary") \
                                .type('reviews')) \
        ).match(pview.widgets)
        self.assertMatchResult(match)
