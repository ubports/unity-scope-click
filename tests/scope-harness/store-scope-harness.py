#!/usr/bin/env python3
# coding: utf-8

from scope_harness import *
from scope_harness.testing import *
import unittest, sys

class StoreTest (ScopeHarnessTestCase):
    @classmethod
    def setUpClass(cls):
        cls.harness = ScopeHarness.new_from_scope_list(Parameters([
            "/usr/lib/arm-linux-gnueabihf/unity-scopes/clickstore/com.canonical.scopes.clickstore.ini"
            ]))

    def setUp(self):
        self.view = self.harness.results_view
        self.view.active_scope = 'com.canonical.scopes.clickstore'

    def test_surfacing_results(self):
        self.view.browse_department('')
        self.view.search_query = ''

        # Check first apps of every category
        match = CategoryListMatcher() \
            .has_exactly(4) \
            .mode(CategoryListMatcherMode.BY_ID) \
            .category(CategoryMatcher("app-of-the-week") \
                    .has_at_least(1) \
                    ) \
            .category(CategoryMatcher("top-apps") \
                      .has_at_least(1) \
                      .mode(CategoryMatcherMode.STARTS_WITH) \
                      .result(ResultMatcher("https://search.apps.ubuntu.com/api/v1/package/com.ubuntu.developer.bobo1993324.udropcabin") \
                      .properties({'installed': False, 'version': '0.2.1', 'price': 0.0, 'price_area':'FREE', 'rating':'☆ 4.2'}) \
                      .title('uDropCabin') \
                      .subtitle('Zhang Boren') \
            )) \
            .category(CategoryMatcher("our-favorite-games") \
                      .has_at_least(1) \
                      .mode(CategoryMatcherMode.BY_URI) \
                      .result(ResultMatcher("https://search.apps.ubuntu.com/api/v1/package/com.ubuntu.developer.andrew-hayzen.volleyball2d") \
            )) \
            .category(CategoryMatcher("travel-apps") \
                      .has_at_least(1)) \
            .match(self.view.categories)
        self.assertMatchResult(match)

    def test_surfacing_departments(self):
        self.view.search_query = ''

        departments = self.view.browse_department('')

        self.assertTrue(self.view.has_departments)
        self.assertFalse(self.view.has_alt_departments)

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

        res_match = CategoryListMatcher() \
            .has_exactly(3) \
            .mode(CategoryListMatcherMode.BY_ID) \
            .category(CategoryMatcher("top-games") \
                      .has_at_least(1) \
            ) \
            .category(CategoryMatcher("__all-scopes__") \
                      .has_at_least(1) \
            ) \
            .category(CategoryMatcher("__all-apps__") \
                      .has_at_least(1) \
            ) \
            .match(self.view.categories)
        self.assertMatchResult(res_match)

    def test_uninstalled_app_preview(self):
        self.view.browse_department('')
        self.view.search_query = 'Calendar'

        pview = self.view.categories[0].results[0].tap()
        self.assertIsInstance(pview, PreviewView)

        match = PreviewColumnMatcher().column(\
                PreviewMatcher() \
                .widget(PreviewWidgetMatcher("hdr")) \
                .widget(PreviewWidgetMatcher("buttons") \
                    .type("actions") \
                    .data(
                        {"actions":[
                            {"download_sha512":"2fa658804e63da1869037cd9bc74b792875404f03b6c6449271ae5244688ff42a4524712ccb748ab9004344cccddd59063f3d3a4af899a3cc6f64ddc1a27072b",
                             "download_url":"https://public.apps.ubuntu.com/download/com.ubuntu/calendar/com.ubuntu.calendar_0.4.572_all.click","id":"install_click","label":"Install"}]
                             ,"online_account_details": {
                                "login_failed_action":1,
                                "login_passed_action":3,
                                "provider_name":"ubuntuone",
                                "scope_id":"",
                                "service_name": "ubuntuone",
                                "service_type":"ubuntuone"}}
                        ) \
                    ) \
                .widget(PreviewWidgetMatcher("screenshots") \
                    .type('gallery')) \
                .widget(PreviewWidgetMatcher("summary") \
                    .type('text')) \
                .widget(PreviewWidgetMatcher("other_metadata") \
                        .type('text')) \
                .widget(PreviewWidgetMatcher("updates") \
                        .type('text')) \
                .widget(PreviewWidgetMatcher("whats_new") \
                        .type('text')) \
                .widget(PreviewWidgetMatcher("reviews_title") \
                        .type('text')) \
                .widget(PreviewWidgetMatcher("summary") \
                        .type('reviews')) \
        ).match(pview.widgets)
        self.assertMatchResult(match)

if __name__ == '__main__':
    unittest.main(argv = sys.argv[:1])
