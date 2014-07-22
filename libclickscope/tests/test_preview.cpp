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

#include <unity/scopes/testing/MockPreviewReply.h>

#include <gtest/gtest.h>
#include <click/preview.h>

using namespace ::testing;
using namespace unity::scopes;

class FakeResult : public Result
{
public:
    FakeResult(VariantMap& vm) : Result(vm)
    {
    }

};

class FakePreview : public click::PreviewStrategy
{
public:
    FakePreview(Result& result) : click::PreviewStrategy::PreviewStrategy(result)
    {

    }

    void run(const PreviewReplyProxy &/*reply*/)
    {

    }
    using click::PreviewStrategy::screenshotsWidgets;
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
