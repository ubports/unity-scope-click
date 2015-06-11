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

#include <json/reader.h>
#include <json/value.h>

#include <glib.h>
#include <libpay/pay-package.h>

#include <QDebug>

namespace json = Json;


struct pay::Package::Private
{
    Private()
    {
    }

    virtual ~Private()
    {
    }

    PayPackage *pay_package;
};


static void pay_verification_observer(PayPackage*,
                                      const char* item_id,
                                      PayPackageItemStatus status,
                                      void* user_data)
{
    pay::Package* p = static_cast<pay::Package*>(user_data);
    std::string callback_id = std::string{item_id} + pay::APPENDAGE_VERIFY;
    if (p->callbacks.count(item_id) == 0) {
        // Do nothing if we don't have a callback registered.
        return;
    }

    switch (status) {
    case PAY_PACKAGE_ITEM_STATUS_PURCHASED:
        p->callbacks[item_id](item_id, true);
        break;
    case PAY_PACKAGE_ITEM_STATUS_NOT_PURCHASED:
        p->callbacks[item_id](item_id, false);
        break;
    default:
        break;
    }
}

static  void pay_refund_observer(PayPackage*,
                                 const char* item_id,
                                 PayPackageRefundStatus status,
                                 void* user_data)
{
    pay::Package* p = static_cast<pay::Package*>(user_data);
    std::string callback_id = std::string{item_id} + pay::APPENDAGE_REFUND;
    if (p->callbacks.count(callback_id) == 0) {
        // Do nothing if we don't have a callback registered.
        return;
    }

    switch (status) {
    case PAY_PACKAGE_REFUND_STATUS_NOT_PURCHASED:
        p->callbacks[callback_id](item_id, true);
        break;
    case PAY_PACKAGE_REFUND_STATUS_NOT_REFUNDABLE:
        p->callbacks[callback_id](item_id, false);
    default:
        break;
    }
}


namespace pay {

bool operator==(const Purchase& lhs, const Purchase& rhs) {
    return lhs.name == rhs.name;
}

Package& Package::instance() {
    static Package the_instance;
    return the_instance;
}

Package::Package() : impl(new Private())
{
}

Package::Package(const QSharedPointer<click::web::Client>& client) :
    impl(new Private()),
    client(client)
{
}

Package::~Package()
{
    if (running) {
        pay_package_item_observer_uninstall(impl->pay_package,
                                            pay_verification_observer,
                                            this);
        pay_package_delete(impl->pay_package);
    }
}

typedef std::pair<std::string, bool> _SuccessTuple;

bool Package::refund(const std::string& pkg_name)
{
    std::promise<_SuccessTuple> result_promise;
    std::future<_SuccessTuple> result_future = result_promise.get_future();
    _SuccessTuple result;

    if (!running) {
        setup_pay_service();
    }

    std::string callback_id = pkg_name + pay::APPENDAGE_REFUND;
    if (callbacks.count(callback_id) == 0) {
        callbacks[callback_id] = [pkg_name,
                                  &result_promise](const std::string& item_id,
                                                   bool succeeded) {
            if (item_id == pkg_name) {
                _SuccessTuple refund_result{item_id, succeeded};
                try {
                    result_promise.set_value(refund_result);
                } catch (std::future_error) {
                    // Just log this to avoid crashing, as it seems that
                    // sometimes this callback may be called more than once.
                    qDebug() << "Refund callback called again for:" << item_id.c_str();
                }
            }
        };
        qDebug() << "Attempting to cancel purchase of " << pkg_name.c_str();
        pay_package_item_start_refund(impl->pay_package, pkg_name.c_str());

        result = result_future.get();

        callbacks.erase(callback_id);

        return result.second;
    }
    return false;
}

bool Package::verify(const std::string& pkg_name)
{
    std::promise<_SuccessTuple> result_promise;
    std::future<_SuccessTuple> result_future = result_promise.get_future();
    _SuccessTuple result;

    std::string callback_id = pkg_name + pay::APPENDAGE_VERIFY;
    if (callbacks.count(callback_id) == 0) {
        callbacks[callback_id] = [pkg_name,
                                  &result_promise](const std::string& item_id,
                                                   bool purchased) {
            if (item_id == pkg_name) {
                _SuccessTuple found_purchase{item_id, purchased};
                try {
                    result_promise.set_value(found_purchase);
                } catch (std::future_error) {
                    // Just log this to avoid crashing, as it seems that
                    // sometimes this callback may be called more than once.
                    qDebug() << "Callback called again for:" << item_id.c_str();
                }
            }
        };
        qDebug() << "Checking if " << pkg_name.c_str() << " was purchased.";
        pay_package_verify(pkg_name);

        result = result_future.get();

        callbacks.erase(callback_id);

        return result.second;
    }
    return false;
}

time_t parse_timestamp(json::Value v)
{
    if (v.isNull()) {
        return 0;
    }

    QDateTime when = QDateTime::fromString(QString::fromStdString(v.asString()), Qt::ISODate);
    when.setTimeSpec(Qt::OffsetFromUTC);

    return when.toTime_t();
}


click::web::Cancellable Package::get_purchases(std::function<void(const PurchaseSet&)> callback)
{
    QSharedPointer<click::CredentialsService> sso(new click::CredentialsService());
    client->setCredentialsService(sso);

    QSharedPointer<click::web::Response> response = client->call
        (get_base_url() + pay::API_ROOT + pay::PURCHASES_API_PATH, "GET", true);

    QObject::connect(response.data(), &click::web::Response::finished,
                     [=](QString reply) {
                         PurchaseSet purchases;
                         json::Reader reader;
                         json::Value root;

                         if (reader.parse(reply.toUtf8().constData(), root)) {
                             for (uint i = 0; i < root.size(); i++) {
                                 const json::Value item = root[i];
                                 if (item[JsonKeys::state].asString() == PURCHASE_STATE_COMPLETE) {
                                     auto package_name = item[JsonKeys::package_name].asString();
                                     qDebug() << "parsing:" << package_name.c_str();
                                     auto refundable_until_value = item[JsonKeys::refundable_until];
                                     qDebug() << "refundable until:" << refundable_until_value.asString().c_str();
                                     auto refundable_parsed = parse_timestamp(refundable_until_value);
                                     qDebug() << "parsed:" << refundable_parsed;
                                     Purchase p(package_name, refundable_parsed);
                                     purchases.insert(p);
                                 }
                             }
                         }
                         callback(purchases);
                     });
    QObject::connect(response.data(), &click::web::Response::error,
                     [=](QString) {
                         qWarning() << "Network error getting purchases.";
                         callback(PurchaseSet());
                     });

    return click::web::Cancellable(response);
}

std::string Package::get_base_url()
{
    const char *env_url = getenv(pay::BASE_URL_ENVVAR);
    if (env_url != NULL) {
        return env_url;
    }
    return pay::BASE_URL;
}

void Package::setup_pay_service()
{
    PayPackage* newpkg = pay_package_new(Package::NAME);
    impl->pay_package = newpkg;

    qDebug() << "installing observers";
    pay_package_item_observer_install(impl->pay_package,
                                      pay_verification_observer,
                                      this);
    pay_package_refund_observer_install(impl->pay_package,
                                        pay_refund_observer,
                                        this);

    running = true;
}

void Package::pay_package_verify(const std::string& pkg_name)
{
    if (!running) {
        setup_pay_service();
    }

    if (callbacks.count(pkg_name) == 0) {
        return;
    }

    pay_package_item_start_verification(impl->pay_package, pkg_name.c_str());
}

} // namespace pay
