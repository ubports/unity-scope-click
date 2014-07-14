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

#include <QDebug>

static std::map<std::string,
                std::function<void(std::string, bool)>> callbacks;


static void pay_verification_observer(PayPackage*,
                                      const char* item_id,
                                      PayPackageItemStatus status,
                                      void*)
{
    qDebug() << "Observer fired for:" << item_id;
    callbacks[item_id](item_id, status == PAY_PACKAGE_ITEM_STATUS_PURCHASED);
}


struct pay::Package::Private
{
    Private()
    {
        pay_package = pay_package_new(Package::NAME);
        pay_package_item_observer_install(pay_package,
                                          pay_verification_observer, this);
    }

    virtual ~Private()
    {
        pay_package_item_observer_uninstall(pay_package,
                                            pay_verification_observer, this);
        pay_package_delete(pay_package);
    }

    void verify(const std::string& pkg_name)
    {
        qDebug() << "Querying verification of:" << pkg_name.c_str();
        pay_package_item_start_verification(pay_package, pkg_name.c_str());
    }

    PayPackage *pay_package;
};

namespace pay {

Package::Package(QObject* parent) :
    QObject(parent),
    impl(new Private())
{
}

bool Package::verify(const std::string& pkg_name)
{
    typedef std::pair<std::string, bool> _PurchasedTuple;
    std::promise<_PurchasedTuple> purchased_promise;
    std::future<_PurchasedTuple> purchased_future = purchased_promise.get_future();
    _PurchasedTuple result;

    callbacks[pkg_name] = [pkg_name,
                           &purchased_promise](const std::string& item_id,
                                               bool purchased) {
        qDebug() << "In callback for:" << item_id.c_str();
        if (item_id == pkg_name) {
            _PurchasedTuple found_purchase{item_id, purchased};
            purchased_promise.set_value(found_purchase);
        }
    };

    impl->verify(pkg_name);

    result = purchased_future.get();

    callbacks.erase(pkg_name);

    return result.second;
}

} // namespace pay
