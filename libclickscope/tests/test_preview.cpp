/*
 * Copyright (C) 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#include <click/preview.h>

#include <click/index.h>
#include <click/interface.h>
#include <click/reviews.h>
#include <fake_json.h>
#include <mock_pay.h>
#include <mock_ubuntu_download_manager.h>

#include <QCoreApplication>
#include <QTimer>

#include <boost/locale/time_zone.hpp>
#include <gtest/gtest.h>
#include <time.h>
#include <unity/scopes/testing/MockPreviewReply.h>
#include <unity/scopes/testing/Result.h>

using namespace ::testing;
using ::testing::Matcher;
using namespace unity::scopes;

class FakeResult : public Result
{
public:
    FakeResult(VariantMap& vm) : Result(vm)
    {
    }

};

class FakeIndex : public click::Index
{
public:
    FakeIndex() {

    }
    click::web::Cancellable get_details(const std::string& /*package_name*/, std::function<void(click::PackageDetails, Error)> callback, bool) override {
        callback(click::PackageDetails(), Error::NetworkError);
        return click::web::Cancellable();
    }

};

class FakeReviews : public click::Reviews
{
public:
    FakeReviews() {

    }

    click::web::Cancellable fetch_reviews(const std::string &/*package_name*/, std::function<void (click::ReviewList, Error)> callback, bool) override {
        callback(click::ReviewList(), Error::NoError);
        return click::web::Cancellable();
    }
};

class FakePreview : public click::PreviewStrategy
{
public:
    FakePreview(Result& result) : click::PreviewStrategy::PreviewStrategy(result)
    {
        index.reset(new FakeIndex());
        reviews.reset(new FakeReviews());
    }

    void run(const PreviewReplyProxy &/*reply*/)
    {

    }

    void run_under_qt(const std::function<void()> &task) {
        // when testing, do not actually run under qt
        task();
    }

    using click::PreviewStrategy::screenshotsWidgets;
    using click::PreviewStrategy::descriptionWidgets;
    using click::PreviewStrategy::build_other_metadata;
    using click::PreviewStrategy::build_updates_table;
    using click::PreviewStrategy::build_whats_new;
    using click::PreviewStrategy::populateDetails;
    using click::PreviewStrategy::isRefundable;
};

class PreviewsBaseTest : public Test
{
public:
    VariantMap vm;
    VariantMap internal;
    VariantMap attrs;

    PreviewsBaseTest()
    {
        attrs["uri"] = "fake://result";
        vm["internal"] = internal;
        vm["attrs"] = attrs;
    }
};

class PreviewStrategyTest : public PreviewsBaseTest
{

};

TEST_F(PreviewStrategyTest, testScreenshotsWidget)
{
    FakeResult result{vm};
    FakePreview preview{result};
    click::PackageDetails details;
    details.main_screenshot_url = "sshot1";
    details.more_screenshots_urls = {"sshot2", "sshot3"};

    auto widgets = preview.screenshotsWidgets(details);
    ASSERT_EQ(widgets.size(), 1);
    auto first_widget = widgets.front();
    auto attributes = first_widget.attribute_values();
    auto sources = attributes["sources"].get_array();
    ASSERT_EQ(sources[0].get_string(), "sshot1");
    ASSERT_EQ(sources[1].get_string(), "sshot2");
    ASSERT_EQ(sources[2].get_string(), "sshot3");
}

TEST_F(PreviewStrategyTest, testEmptyResults)
{
    FakeResult result{vm};
    result["name"] = "fake name";
    FakePreview preview{result};
    preview.populateDetails(
                [](const click::PackageDetails &){
                },
                [](const click::ReviewList&, click::Reviews::Error){
                });

}

MATCHER_P(PreviewWidgetsListMatchers, widgets, "") {
    if (arg.size() == widgets.size()) {
        auto it1 = widgets.begin();
        auto it2 = arg.begin();
        while (*it1 != it2->id()) {
            *result_listener << "Preview widgets don't match: " << it2->id() << ", expected " << *it1;
            return false;
        }
        return true;
    }
    *result_listener << "Preview widgets list don't match: ";
    for (auto const& widget: arg) {
        *result_listener << widget.id() << ", ";
    }
    *result_listener << "expected: ";
    for (auto const& widget: widgets) {
        *result_listener << widget << ", ";
    }
    return false;
}

MATCHER_P(LayoutMatches, layouts, "") {
    if (arg.size() != layouts.size()) {
        *result_listener << "Layout lists sizes don't match, " << arg.size() << " vs " << layouts.size();
        return false;
    }
    auto it1 = layouts.begin();
    auto it2 = arg.begin();
    while (it1 != layouts.end()) {
        if (it1->serialize() != it2->serialize()) {
            *result_listener << "Layouts don't match: " << unity::scopes::Variant(it1->serialize()).serialize_json() << " vs "
                << unity::scopes::Variant(it2->serialize()).serialize_json();
            return false;
        }
        it1++;
        it2++;
    }
    return true;
}

TEST_F(PreviewStrategyTest, testPushCachedWidgets)
{
    unity::scopes::testing::MockPreviewReply reply;
    std::shared_ptr<unity::scopes::testing::MockPreviewReply> replyptr{&reply, [](unity::scopes::testing::MockPreviewReply*){}};

    FakeResult result{vm};
    FakePreview preview{result};
    click::PackageDetails details;
    details.main_screenshot_url = "sshot1";
    details.license = "GPL";
    details.company_name = "Ubuntu";
    details.website = "http://ubuntu.com";
    details.changelog = "Foo";
    details.version = "0.1";
    details.download_url = "http://ubuntu.com/";
    details.description = "Foo";

    click::CachedPreviewWidgets cache;
    scopes::PreviewWidget buttons("buttons", "actions");

    unity::scopes::ColumnLayout single_column(1);
    single_column.add_column({"hdr","buttons","screenshots","summary","other_metadata","updates_table","whats_new"});
    unity::scopes::ColumnLayout two_columns(2);
    two_columns.add_column({"hdr","buttons","screenshots","summary"});
    two_columns.add_column({"other_metadata","updates_table","whats_new"});
    unity::scopes::ColumnLayoutList expected_layout {single_column, two_columns};

    std::vector<std::string> expected_widgets {"hdr", "buttons", "screenshots", "summary", "other_metadata", "updates_table", "whats_new"};

    EXPECT_CALL(*replyptr, register_layout(Matcher<unity::scopes::ColumnLayoutList const&>(LayoutMatches(expected_layout))));
    EXPECT_CALL(*replyptr, push(Matcher<unity::scopes::PreviewWidgetList const&>(PreviewWidgetsListMatchers(expected_widgets))));
    preview.pushPackagePreviewWidgets(cache, details, {buttons});
    cache.flush(replyptr);
}

class PreviewStrategyDescriptionTest : public PreviewStrategyTest
{
public:
    FakeResult result{vm};
    FakePreview preview{result};
    click::PackageDetails details;
    unity::scopes::PreviewWidgetList widgets;

    PreviewStrategyDescriptionTest()
    {
    }
    void assertWidgetAttribute(int n, std::string attribute_name, std::string expected_value)
    {
        ASSERT_GT(widgets.size(), n);
        auto widget = std::next(widgets.begin(), n);
        auto attributes = widget->attribute_values();
        ASSERT_EQ(expected_value, attributes[attribute_name].get_string());
    }
    void assertWidgetNestedVariantArray(int n, int x, std::string expected_title, std::string expected_value)
    {
        auto widget = std::next(widgets.begin(), n);
        auto attributes = widget->attribute_values();
        auto found_title = attributes["values"].get_array()[x].get_array()[0].get_string();
        ASSERT_EQ(found_title, expected_title);
        auto found_value = attributes["values"].get_array()[x].get_array()[1].get_string();
        ASSERT_EQ(found_value, expected_value);
    }
};

TEST_F(PreviewStrategyDescriptionTest, testDescriptionWidgetsFull)
{
    boost::locale::time_zone::global("UTC");
    details = click::PackageDetails::from_json(FAKE_JSON_PACKAGE_DETAILS);
    widgets = preview.descriptionWidgets(details);

    assertWidgetAttribute(0, "title", "Info");
    assertWidgetAttribute(0, "text", details.description);

    assertWidgetNestedVariantArray(1, 0, "Publisher/Creator", "Fake Publisher");
    assertWidgetNestedVariantArray(1, 1, "Seller", "Fake Company");
    assertWidgetNestedVariantArray(1, 2, "Website", "http://example.com");
    assertWidgetNestedVariantArray(1, 3, "Contact", "http://example.com/support");
    assertWidgetNestedVariantArray(1, 4, "License", "Proprietary");

    assertWidgetAttribute(2, "title", "Updates");
    assertWidgetNestedVariantArray(2, 0, "Version number", "0.2");

    assertWidgetNestedVariantArray(2, 1, "Last updated", "Jul 3, 2014");
    assertWidgetNestedVariantArray(2, 2, "First released", "Nov 4, 2013");
    assertWidgetNestedVariantArray(2, 3, "Size", "173.4 KiB");

    assertWidgetAttribute(3, "title", "What's new");
    assertWidgetAttribute(3, "text", preview.build_whats_new(details));

}

TEST_F(PreviewStrategyDescriptionTest, testDescriptionWidgetsMinimal)
{
    details = click::PackageDetails::from_json(FAKE_JSON_PACKAGE_DETAILS_DEB);
    widgets = preview.descriptionWidgets(details);

    ASSERT_EQ(1, widgets.size());

    assertWidgetAttribute(0, "title", "Info");
    assertWidgetAttribute(0, "text", details.description);
}

TEST_F(PreviewStrategyDescriptionTest, testDescriptionWidgetsNone)
{
    widgets = preview.descriptionWidgets(details);
    ASSERT_EQ(0, widgets.size());
}

class FakePreviewStrategy :  public click::PreviewStrategy
{
public:
    FakePreviewStrategy(const unity::scopes::Result& result) : PreviewStrategy(result) {}
    using click::PreviewStrategy::pushPackagePreviewWidgets;
    std::vector<std::string> call_order;
    virtual void run(unity::scopes::PreviewReplyProxy const& /*reply*/)
    {
    }

    virtual unity::scopes::PreviewWidgetList screenshotsWidgets (const click::PackageDetails &/*details*/) {
        call_order.push_back("screenshots");
        return {};
    }
    virtual unity::scopes::PreviewWidgetList headerWidgets (const click::PackageDetails &/*details*/) {
        call_order.push_back("header");
        return {};
    }
};

TEST_F(PreviewStrategyTest, testScreenshotsPushedAfterHeader)
{
    FakeResult result{vm};
    unity::scopes::testing::MockPreviewReply reply;
    std::shared_ptr<unity::scopes::testing::MockPreviewReply> replyptr{&reply, [](unity::scopes::testing::MockPreviewReply*){}};
    click::PackageDetails details;

    FakePreviewStrategy preview{result};
    preview.pushPackagePreviewWidgets(replyptr, details, {});
    std::vector<std::string> expected_order {"header", "screenshots"};
    ASSERT_EQ(expected_order, preview.call_order);
}

class StrategyChooserTest : public Test {
protected:
    unity::scopes::testing::Result result;
    unity::scopes::ActionMetadata metadata;
    unity::scopes::VariantMap metadict;
    QSharedPointer<click::web::Client> client;
    QSharedPointer<MockPayPackage> pay_package;
    QSharedPointer<MockSystemDownloadManager> dm;
    std::shared_ptr<click::DepartmentsDb> depts;
    const std::string FAKE_SHA512 = "FAKE_SHA512";

public:
    StrategyChooserTest() : metadata("en_EN", "phone") {
    }
};

class MockablePreview : public click::Preview {
public:
    MockablePreview(const unity::scopes::Result& result, const unity::scopes::ActionMetadata& metadata) :
        click::Preview(result, metadata)
    {

    }

    MOCK_METHOD6(build_installing,
                 click::PreviewStrategy*(const std::string&,
                                         const std::string&,
                                         const unity::scopes::Result&,
                                         const QSharedPointer<click::web::Client>&,
                                         const QSharedPointer<Ubuntu::DownloadManager::Manager>&,
                                         std::shared_ptr<click::DepartmentsDb>));
};

TEST_F(StrategyChooserTest, testSha512IsUsed) {
    metadict["action_id"] = click::Preview::Actions::INSTALL_CLICK;
    metadict["download_url"] = "fake_download_url";
    metadict["download_sha512"] = FAKE_SHA512;
    metadata.set_scope_data(unity::scopes::Variant(metadict));
    MockablePreview preview(result, metadata);
    EXPECT_CALL(preview, build_installing(_, FAKE_SHA512, _, _, _, _));
    preview.choose_strategy(client, pay_package, dm, depts);
}


class UninstalledPreviewTest : public PreviewsBaseTest {
public:
    FakeResult result{vm};
    click::PackageDetails details;
    unity::scopes::PreviewWidgetList widgets;
    QSharedPointer<click::web::Client> client;
    QSharedPointer<MockPayPackage> pay_package;
    QSharedPointer<MockSystemDownloadManager> sdm;
    std::shared_ptr<click::DepartmentsDb> depts;
    unity::scopes::testing::MockPreviewReply reply;
    std::shared_ptr<unity::scopes::testing::MockPreviewReply> replyptr{&reply, [](unity::scopes::testing::MockPreviewReply*){}};
};

class FakeBaseUninstalledPreview : public click::UninstalledPreview {
    std::string object_path;
public:
    FakeBaseUninstalledPreview(const std::string& object_path,
                               const unity::scopes::Result& result,
                               const unity::scopes::ActionMetadata& metadata,
                               const QSharedPointer<click::web::Client>& client,
                               const std::shared_ptr<click::DepartmentsDb>& depts,
                               const QSharedPointer<Ubuntu::DownloadManager::Manager>& manager,
                               const QSharedPointer<pay::Package> pay_package)
        : click::UninstalledPreview(result, metadata, client, depts, manager, pay_package),
          object_path(object_path)
    {
    }

    void populateDetails(std::function<void (const click::PackageDetails &)> details_callback,
                         std::function<void (const click::ReviewList &, click::Reviews::Error)> reviews_callback) {
        click::PackageDetails details;
        details_callback(details);
        reviews_callback({}, click::Reviews::Error::NoError);
    }
};

class FakeUninstalledPreview : public FakeBaseUninstalledPreview {
public:
    MOCK_METHOD1(uninstalledActionButtonWidgets, scopes::PreviewWidgetList (const click::PackageDetails &details));
    MOCK_METHOD1(progressBarWidget, scopes::PreviewWidgetList(const std::string& object_path));
    FakeUninstalledPreview(const std::string& object_path,
                           const unity::scopes::Result& result,
                           const unity::scopes::ActionMetadata& metadata,
                           const QSharedPointer<click::web::Client>& client,
                           const std::shared_ptr<click::DepartmentsDb>& depts,
                           const QSharedPointer<Ubuntu::DownloadManager::Manager>& manager,
                           const QSharedPointer<pay::Package> pay_package)
        : FakeBaseUninstalledPreview(object_path, result, metadata, client, depts, manager, pay_package) {
    }
};


// FIXME: Needs Qt main loop
TEST_F(UninstalledPreviewTest, DISABLED_testDownloadInProgress) {
    std::string fake_object_path = "/fake/object/path";

    result["name"] = "fake_app_name";
    scopes::PreviewWidgetList response;
    unity::scopes::ActionMetadata metadata("en_EN", "desktop");
    FakeUninstalledPreview preview(fake_object_path, result, metadata, client, depts, sdm, pay_package);
    EXPECT_CALL(preview, progressBarWidget(_))
            .Times(1)
            .WillOnce(Return(response));
    EXPECT_CALL(*replyptr, register_layout(_));
    preview.run(replyptr);
}

// FIXME: Needs Qt main loop
TEST_F(UninstalledPreviewTest, DISABLED_testNoDownloadProgress) {
    std::string fake_object_path = "";

    result["name"] = "fake_app_name";
    scopes::PreviewWidgetList response;
    unity::scopes::ActionMetadata metadata("en_EN", "desktop");
    FakeUninstalledPreview preview(fake_object_path, result, metadata, client, depts, sdm, pay_package);
    EXPECT_CALL(preview, uninstalledActionButtonWidgets(_))
            .Times(1)
            .WillOnce(Return(response));
    preview.run(replyptr);
}

class FakeUninstalledRefundablePreview : FakeBaseUninstalledPreview {
public:
    FakeUninstalledRefundablePreview(const unity::scopes::Result& result,
                                     const unity::scopes::ActionMetadata& metadata,
                                     const QSharedPointer<click::web::Client>& client,
                                     const std::shared_ptr<click::DepartmentsDb>& depts,
                                     const QSharedPointer<Ubuntu::DownloadManager::Manager>& manager,
                                     const QSharedPointer<pay::Package> pay_package)
        : FakeBaseUninstalledPreview(std::string{""}, result, metadata, client, depts, manager, pay_package){
    }
    using click::UninstalledPreview::uninstalledActionButtonWidgets;
    MOCK_METHOD0(isRefundable, bool());
};

unity::scopes::VariantArray get_actions_from_widgets(const unity::scopes::PreviewWidgetList& widgets, int widget_number) {
    auto widget = *std::next(widgets.begin(), widget_number);
    return widget.attribute_values()["actions"].get_array();
}

std::string get_action_from_widgets(const unity::scopes::PreviewWidgetList& widgets, int widget_number, int action_number) {
    auto actions = get_actions_from_widgets(widgets, widget_number);
    auto selected_action = actions.at(action_number).get_dict();
    return selected_action["id"].get_string();
}

TEST_F(UninstalledPreviewTest, testPaymentsButtonNoAccountsData) {
    result["name"] = "fake_app_name";
    result["price"] = 2.99;
    result["purchased"] = false;
    result["currency_symbol"] = "USD";
    FakeBaseUninstalledPreview preview("", result, client, depts, sdm, pay_package);
    click::PackageDetails pkgdetails;
    pkgdetails.package = click::Package(result["name"].get_string(), "0.1");
    pkgdetails.download_url = "https://localhost/example/fake_app_name.click";
    pkgdetails.download_sha512 = "download_sha512";
    auto widgets = preview.uninstalledActionButtonWidgets(pkgdetails);
    auto button = *std::next(widgets.begin(), 0);
    std::cerr << button.data() << std::endl;
    ASSERT_EQ(button.attribute_values().count("online_account_details"), 0);
}

TEST_F(UninstalledPreviewTest, testIsRefundableButtonShown) {
    result["name"] = "fake_app_name";
    result["price"] = 2.99;
    result["purchased"] = true;
    unity::scopes::ActionMetadata metadata("en_EN", "desktop");
    FakeUninstalledRefundablePreview preview(result, metadata, client, depts, sdm, pay_package);

    click::PackageDetails pkgdetails;
    EXPECT_CALL(preview, isRefundable()).Times(1)
            .WillOnce(Return(true));
    auto widgets = preview.uninstalledActionButtonWidgets(pkgdetails);
    ASSERT_EQ(get_action_from_widgets(widgets, 0, 1), "cancel_purchase_uninstalled");
}

TEST_F(UninstalledPreviewTest, testIsRefundableButtonNotShown) {
    result["name"] = "fake_app_name";
    result["price"] = 2.99;
    result["purchased"] = true;
    unity::scopes::ActionMetadata metadata("en_EN", "desktop");
    FakeUninstalledRefundablePreview preview(result, metadata, client, depts, sdm, pay_package);

    click::PackageDetails pkgdetails;
    EXPECT_CALL(preview, isRefundable()).Times(1)
            .WillOnce(Return(false));
    auto widgets = preview.uninstalledActionButtonWidgets(pkgdetails);
    ASSERT_EQ(get_actions_from_widgets(widgets, 0).size(), 1);
}

class InstalledPreviewTest : public Test {
protected:
    unity::scopes::testing::Result result;
    unity::scopes::ActionMetadata metadata;
    unity::scopes::VariantMap metadict;
    QSharedPointer<click::web::Client> client;
    QSharedPointer<MockPayPackage> pay_package;
    std::shared_ptr<click::DepartmentsDb> depts;

public:
    InstalledPreviewTest() : metadata("en_EN", "phone") {
    }
};

class FakeInstalledRefundablePreview : public click::InstalledPreview  {
public:
    FakeInstalledRefundablePreview(const unity::scopes::Result& result,
                                   const unity::scopes::ActionMetadata& metadata,
                                   const QSharedPointer<click::web::Client> client,
                                   const QSharedPointer<pay::Package> pay_package,
                                   const std::shared_ptr<click::DepartmentsDb> depts)
        : click::InstalledPreview(result, metadata, client, pay_package, depts) {

    }

    using click::InstalledPreview::createButtons;
    MOCK_METHOD0(isRefundable, bool());
};

TEST_F(InstalledPreviewTest, testIsRefundableButtonShownFromStore) {
    result["price"] = 3.99f;
    FakeInstalledRefundablePreview preview(result, metadata, client, pay_package, depts);
    EXPECT_CALL(preview, isRefundable()).Times(1)
            .WillOnce(Return(true));
    click::Manifest manifest;
    manifest.removable = true;
    auto widgets = preview.createButtons("fake uri", manifest);
    ASSERT_EQ(get_actions_from_widgets(widgets, 0).size(), 2);
    ASSERT_EQ(get_action_from_widgets(widgets, 0, 1), "cancel_purchase_installed");
}

TEST_F(InstalledPreviewTest, testIsRefundableButtonNotShownFromApps) {
    FakeInstalledRefundablePreview preview(result, metadata, client, pay_package, depts);
    EXPECT_CALL(preview, isRefundable()).Times(0);
    click::Manifest manifest;
    manifest.removable = true;
    auto widgets = preview.createButtons("fake uri", manifest);
    ASSERT_EQ(get_actions_from_widgets(widgets, 0).size(), 2);
    ASSERT_EQ(get_action_from_widgets(widgets, 0, 1), "uninstall_click");
}

TEST_F(InstalledPreviewTest, testIsRefundableButtonNotShown) {
    FakeInstalledRefundablePreview preview(result, metadata, client, pay_package, depts);
    EXPECT_CALL(preview, isRefundable()).Times(0);
    click::Manifest manifest;
    manifest.removable = true;
    click::PackageDetails details;
    auto widgets = preview.createButtons("fake uri", manifest);
    ASSERT_EQ(get_actions_from_widgets(widgets, 0).size(), 2);
    ASSERT_EQ(get_action_from_widgets(widgets, 0, 1), "uninstall_click");
}

class FakeCancelPurchasePreview : public click::CancelPurchasePreview  {
public:
    FakeCancelPurchasePreview(const unity::scopes::Result& result, bool installed)
        : click::CancelPurchasePreview(result, installed) {

    }
    using click::CancelPurchasePreview::build_widgets;
};

class CancelPurchasePreviewTest : public PreviewsBaseTest {

};

TEST_F(CancelPurchasePreviewTest, testNoShowsInstalled)
{
    FakeResult result{vm};
    result["title"] = "fake app";
    FakeCancelPurchasePreview preview(result, true);
    auto widgets = preview.build_widgets();
    auto action = get_action_from_widgets(widgets, 1, 0);
    ASSERT_EQ(action, "show_installed");
}

TEST_F(CancelPurchasePreviewTest, testNoShowsUninstalled)
{
    FakeResult result{vm};
    result["title"] = "fake app";
    FakeCancelPurchasePreview preview(result, false);
    auto widgets = preview.build_widgets();
    auto action = get_action_from_widgets(widgets, 1, 0);
    ASSERT_EQ(action, "show_uninstalled");
}

TEST_F(CancelPurchasePreviewTest, testYesCancelsPurchase)
{
    FakeResult result{vm};
    result["title"] = "fake app";
    FakeCancelPurchasePreview preview(result, false);
    auto widgets = preview.build_widgets();
    auto action = get_action_from_widgets(widgets, 1, 1);
    ASSERT_EQ(action, "confirm_cancel_purchase_uninstalled");
}
