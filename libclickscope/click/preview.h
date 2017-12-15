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

#ifndef CLICKPREVIEW_H
#define CLICKPREVIEW_H

#include <click/index.h>
#include <click/qtbridge.h>

#include <unity/scopes/ActionMetadata.h>
#include <unity/scopes/PreviewQueryBase.h>
#include <unity/scopes/PreviewWidget.h>
#include <unity/scopes/Result.h>
#include <unity/scopes/ScopeBase.h>
#include <unity/util/DefinesPtrs.h>
#include <set>

namespace scopes = unity::scopes;

namespace click {

class Manifest;
class PreviewStrategy;
class DepartmentsDb;

struct WidgetsInColumns
{
    struct {
        std::vector<std::string> column1;
    } singleColumn;
    struct {
        std::vector<std::string> column1;
        std::vector<std::string> column2;
    } twoColumns;

    void registerLayouts(unity::scopes::PreviewReplyProxy const& reply);
    void appendToColumn(std::vector<std::string>& column, unity::scopes::PreviewWidgetList const& widgets);
};

class DepartmentUpdater
{
protected:
    DepartmentUpdater() = default;
    DepartmentUpdater(const std::shared_ptr<click::DepartmentsDb>& depts);
    virtual ~DepartmentUpdater() = default;

private:
    std::shared_ptr<click::DepartmentsDb> depts;
};

class Preview : public unity::scopes::PreviewQueryBase
{
protected:
    std::unique_ptr<PreviewStrategy> strategy;
    const unity::scopes::Result& result;
    const unity::scopes::ActionMetadata& metadata;
    std::shared_future<void> qt_ready_;
    PreviewStrategy* build_strategy(const unity::scopes::Result& result,
                                    const unity::scopes::ActionMetadata& metadata,
                                    std::shared_ptr<click::DepartmentsDb> depts);
public:
    UNITY_DEFINES_PTRS(Preview);
    struct Actions
    {
        Actions() = delete;

        constexpr static const char* OPEN_CLICK{"open_click"};
        constexpr static const char* PIN_TO_LAUNCHER{"pin_to_launcher"};
        constexpr static const char* UNINSTALL_CLICK{"uninstall_click"};
        constexpr static const char* CONFIRM_UNINSTALL{"confirm_uninstall"};
        constexpr static const char* SHOW_UNINSTALLED{"show_uninstalled"};
        constexpr static const char* SHOW_INSTALLED{"show_installed"};
    };

    Preview(const unity::scopes::Result& result);
    Preview(const unity::scopes::Result& result,
            const unity::scopes::ActionMetadata& metadata,
            std::shared_future<void> const& qt_ready = std::future<void>());
    virtual ~Preview();
    void choose_strategy(std::shared_ptr<click::DepartmentsDb> depts);
    // From unity::scopes::PreviewQuery
    void cancelled() override;
    virtual void run(unity::scopes::PreviewReplyProxy const& reply) override;
};

class PreviewStrategy
{
public:

    PreviewStrategy(const unity::scopes::Result& result);
    virtual ~PreviewStrategy();

    virtual void run(unity::scopes::PreviewReplyProxy const& reply) = 0;

    virtual void run_under_qt(const std::function<void ()> &task);
    virtual void invalidateScope(const std::string& scope_id);

protected:
    virtual void populateDetails(std::function<void(const PackageDetails &)> details_callback);
    virtual scopes::PreviewWidgetList headerWidgets(const PackageDetails &details);
    virtual void pushPackagePreviewWidgets(const unity::scopes::PreviewReplyProxy &reply,
                                           const PackageDetails& details,
                                           const scopes::PreviewWidgetList& button_area_widgets);

    scopes::Result result;

    QSharedPointer<click::Index> index;
};

class InstalledPreview : public PreviewStrategy, public DepartmentUpdater
{
public:
    InstalledPreview(const unity::scopes::Result& result,
                     const unity::scopes::ActionMetadata& metadata,
                     const std::shared_ptr<click::DepartmentsDb>& depts);

    virtual ~InstalledPreview();

    void run(unity::scopes::PreviewReplyProxy const& reply) override;

    std::string getApplicationUri(const Manifest& manifest);
    scopes::PreviewWidgetList createButtons(const click::Manifest& manifest);

private:
    scopes::ActionMetadata metadata;
};

class InstalledScopePreview : public PreviewStrategy
{
public:
    InstalledScopePreview(const unity::scopes::Result& result);
    void run(unity::scopes::PreviewReplyProxy const& reply) override;
};

class UninstallConfirmationPreview : public PreviewStrategy
{
public:
    UninstallConfirmationPreview(const unity::scopes::Result& result);

    virtual ~UninstallConfirmationPreview();

    void run(unity::scopes::PreviewReplyProxy const& reply) override;
};

class UninstalledPreview : public PreviewStrategy, public DepartmentUpdater
{
public:
    UninstalledPreview(const unity::scopes::Result& result,
                       const unity::scopes::ActionMetadata& metadata,
                       const std::shared_ptr<click::DepartmentsDb>& depts);

    virtual ~UninstalledPreview();

    void run(unity::scopes::PreviewReplyProxy const& reply) override;
    virtual scopes::PreviewWidgetList uninstalledActionButtonWidgets(const PackageDetails &details);

protected:
    scopes::ActionMetadata metadata;
    PackageDetails found_details;
    std::string found_object_path;
};

// TODO: this is only necessary to perform uninstall.
// That should be moved to a separate action, and this class removed.
class UninstallingPreview : public UninstalledPreview
{
public:
    UninstallingPreview(const unity::scopes::Result& result,
                        const unity::scopes::ActionMetadata& metadata);

    virtual ~UninstallingPreview();

    void run(unity::scopes::PreviewReplyProxy const& reply) override;

protected:
    void uninstall();

};

} // namespace click

#endif
