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
import testtools

from . import ScopeTestBase
from scope_harness import (
    CategoryListMatcher,
    CategoryListMatcherMode,
    CategoryMatcher,
    CategoryMatcherMode,
    ChildDepartmentMatcher,
    DepartmentMatcher,
    DepartmentMatcherMode,
    Parameters,
    PreviewColumnMatcher,
    PreviewMatcher,
    PreviewView,
    PreviewWidgetMatcher,
    ResultMatcher,
    ScopeHarness,
)


class StoreTest(ScopeTestBase):
    def setUp(self):
        super().setUp()
        scope_dir = os.environ.get('SCOPE_DIR', None)
        self.harness = self.launch_scope(scope_dir)
        self.view = self.harness.results_view
        self.view.active_scope = 'com.canonical.scopes.clickstore'

    def launch_scope(self, scope_dir=None):
        """Find the scope and launch it."""
        if scope_dir is None:
            return ScopeHarness.new_from_scope_list(Parameters([
                "com.canonical.scopes.clickstore.ini"
            ]))
        else:
            scope_path = os.path.join(
                scope_dir, 'clickstore',
                'com.canonical.scopes.clickstore.ini')
            return ScopeHarness.new_from_scope_list(Parameters([
                scope_path
            ]))

    def test_surfacing_results(self):
        self.view.browse_department('')
        self.view.search_query = ''
        cpi_base = os.environ["U1_SEARCH_BASE_URL"]

        # Check first apps of every category
        match = (
            CategoryListMatcher()
            .has_exactly(4)
            .mode(CategoryListMatcherMode.BY_ID)
            .category(CategoryMatcher("app-of-the-week")
                      .has_at_least(1))
            .category(
                CategoryMatcher("top-apps")
                .has_at_least(1)
                .mode(CategoryMatcherMode.STARTS_WITH)
                .result(
                    ResultMatcher(
                        cpi_base +
                        '/api/v1/package/' +
                        'com.ubuntu.developer.bobo1993324.udropcabin')
                    .properties(
                        {
                            'installed': False,
                            'version': '0.2.1',
                            'price': 0.0,
                            'price_area': 'FREE',
                            'rating': '☆ 4.2'
                        })
                    .title('uDropCabin')
                    .subtitle('Zhang Boren')
                )
            )
            .category(
                CategoryMatcher("our-favorite-games")
                .has_at_least(1)
                .mode(CategoryMatcherMode.BY_URI)
                .result(ResultMatcher(
                    cpi_base + '/api/v1/package/' +
                    'com.ubuntu.developer.andrew-hayzen.volleyball2d'))
            )
            .category(CategoryMatcher("travel-apps")
                      .has_at_least(1))
            .match(self.view.categories)
        )
        self.assertMatchResult(match)

    def test_surfacing_departments(self):
        self.view.search_query = ''

        departments = self.view.browse_department('')

        self.assertTrue(self.view.has_departments)

        match = DepartmentMatcher() \
            .mode(DepartmentMatcherMode.STARTS_WITH) \
            .id('') \
            .label('All') \
            .all_label('') \
            .parent_id('') \
            .parent_label('') \
            .is_root(True) \
            .is_hidden(False) \
            .child(ChildDepartmentMatcher('books-comics')) \
            .child(ChildDepartmentMatcher('business')) \
            .child(ChildDepartmentMatcher('communication')) \
            .child(ChildDepartmentMatcher('developer-tools')) \
            .child(ChildDepartmentMatcher('education')) \
            .child(ChildDepartmentMatcher('entertainment')) \
            .child(ChildDepartmentMatcher('finance')) \
            .child(ChildDepartmentMatcher('food-drink')) \
            .child(ChildDepartmentMatcher('games')) \
            .child(ChildDepartmentMatcher('graphics')) \
            .child(ChildDepartmentMatcher('health-fitness')) \
            .child(ChildDepartmentMatcher('lifestyle')) \
            .child(ChildDepartmentMatcher('media-video')) \
            .child(ChildDepartmentMatcher('medical')) \
            .child(ChildDepartmentMatcher('music-audio')) \
            .child(ChildDepartmentMatcher('news-magazines')) \
            .child(ChildDepartmentMatcher('personalisation')) \
            .child(ChildDepartmentMatcher('productivity')) \
            .child(ChildDepartmentMatcher('reference')) \
            .child(ChildDepartmentMatcher('science-engineering')) \
            .child(ChildDepartmentMatcher('shopping')) \
            .child(ChildDepartmentMatcher('social-networking')) \
            .child(ChildDepartmentMatcher('sports')) \
            .child(ChildDepartmentMatcher('travel-local')) \
            .child(ChildDepartmentMatcher('universal-accessaccessibility')) \
            .child(ChildDepartmentMatcher('accessories')) \
            .child(ChildDepartmentMatcher('weather')) \
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

        res_match = (
            CategoryListMatcher()
            .has_exactly(3)
            .mode(CategoryListMatcherMode.BY_ID).category(
                CategoryMatcher("top-games")
                .has_at_least(1))
            .category(CategoryMatcher("__all-scopes__")
                      .has_at_least(1))
            .category(CategoryMatcher("__all-apps__")
                      .has_at_least(1))
            .match(self.view.categories)
        )
        self.assertMatchResult(res_match)

    @testtools.skip('Fails only under adt-run.')
    def test_uninstalled_app_preview(self):
        self.view.browse_department('')
        self.view.search_query = 'Calendar'

        pview = self.view.categories[0].results[0].tap()
        self.assertIsInstance(pview, PreviewView)

        match = PreviewColumnMatcher().column(
            PreviewMatcher()
            .widget(PreviewWidgetMatcher("hdr"))
            .widget(
                PreviewWidgetMatcher("buttons")
                .type("actions")
                .data(
                    {
                        "actions": [
                            {
                                'download_sha512': '2fa658804e63da1869037cd' +
                                '9bc74b792875404f03b6c6449271ae5244688ff42a' +
                                '4524712ccb748ab9004344cccddd59063f3d3a4af8' +
                                '99a3cc6f64ddc1a27072b',
                                "download_url": 'https://' +
                                'public.apps.ubuntu.com/download/' +
                                'com.ubuntu/calendar/' +
                                'com.ubuntu.calendar_0.4.572_all.click',
                                "id": "install_click",
                                "label": "Install"
                            }
                        ],
                        "online_account_details": {
                            "login_failed_action": 1,
                            "login_passed_action": 3,
                            "provider_name": "ubuntuone",
                            "scope_id": "",
                            "service_name": "ubuntuone",
                            "service_type": "ubuntuone"
                        }
                    }
                )
            )
            .widget(PreviewWidgetMatcher("screenshots")
                    .type('gallery'))
            .widget(PreviewWidgetMatcher("summary")
                    .type('text'))
            .widget(PreviewWidgetMatcher("other_metadata")
                    .type('table'))
            .widget(PreviewWidgetMatcher("updates_table")
                    .type('table'))
            .widget(PreviewWidgetMatcher("whats_new")
                    .type('text'))
            .widget(PreviewWidgetMatcher("reviews_title")
                    .type('text'))
            .widget(PreviewWidgetMatcher("summary")
                    .type('reviews'))
        ).match(pview.widgets)
        self.assertMatchResult(match)
