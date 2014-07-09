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

#include "pay.h"

#include <future>

#include <glib.h>
#include <libpay/pay-package.h>

struct pay::Package::Private
{
    Private()
    {
        pay_package = pay_package_new(Package::NAME);
    }

    virtual ~Private()
    {
        pay_package_delete(pay_package);
    }
    PayPackage *pay_package;
};

namespace pay {

Package::Package(QObject* parent) :
    QObject(parent),
    impl()
{
}

struct _CallbackData {
    std::function<void(PayPackage*, const char*, PayPackageItemStatus)> callback;
};

static void pay_verification_observer(PayPackage* pay_package,
                                      const char* item_id,
                                      PayPackageItemStatus status,
                                      void* user_data)
{
    struct _CallbackData* cb_data = static_cast<struct _CallbackData*>(user_data);
    cb_data->callback(pay_package, item_id, status);
}

bool Package::verify(const std::string& pkg_name)
{
    std::promise<bool> purchased_promise;
    std::future<bool> purchased_future = purchased_promise.get_future();
    bool purchased = false;

    auto verify_cb = [&](PayPackage*,
                         const char* item_id,
                         PayPackageItemStatus status) {
        if (item_id == pkg_name) {
            if (status == PAY_PACKAGE_ITEM_STATUS_PURCHASED) {
                purchased_promise.set_value(true);
            } else {
                purchased_promise.set_value(false);
            }
        }
    };
    struct _CallbackData *_data = g_new0(struct _CallbackData, 1);
    _data->callback = verify_cb;

    pay_package_item_observer_install(impl->pay_package,
                                      pay_verification_observer, _data);
    pay_package_item_start_verification(impl->pay_package, pkg_name.c_str());

    purchased = purchased_future.get();

    pay_package_item_observer_uninstall(impl->pay_package,
                                        pay_verification_observer, _data);
    free(_data);

    return purchased;
}

} // namespace pay
