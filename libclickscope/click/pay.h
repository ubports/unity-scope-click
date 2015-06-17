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

#ifndef _PAY_H_
#define _PAY_H_

#include <click/webclient.h>

#include <ctime>
#include <map>
#include <memory>
#include <unordered_set>


namespace pay
{
    constexpr static const char* APPENDAGE_VERIFY{":verify"};
    constexpr static const char* APPENDAGE_REFUND{":refund"};


    struct Purchase
    {
        std::string name;
        time_t refundable_until;

        Purchase() = default;
        Purchase(const std::string &name) : name(name), refundable_until(0)
        {
        }

        Purchase(const std::string& name, time_t refundable_until) :
            name(name), refundable_until(refundable_until)
        {
        }

        struct hash_name {
        public :
            size_t operator()(const Purchase &purchase ) const
            {
                return std::hash<std::string>()(purchase.name);
            }
        };

    };

    bool operator==(const Purchase& lhs, const Purchase& rhs);

    typedef std::unordered_set<Purchase, Purchase::hash_name> PurchaseSet;

    typedef std::function<void(const std::string& item_id,
                               bool status)> StatusFunction;

    constexpr static const char* BASE_URL_ENVVAR{"PAY_BASE_URL"};
    constexpr static const char* BASE_URL{"https://software-center.ubuntu.com"};
    constexpr static const char* API_ROOT{"/api/2.0/click/"};
    constexpr static const char* PURCHASES_API_PATH{"purchases/"};
    constexpr static const char* PURCHASE_STATE_COMPLETE{"Complete"};

    struct JsonKeys
    {
        JsonKeys() = delete;

        constexpr static const char* package_name{"package_name"};
        constexpr static const char* refundable_until{"refundable_until"};
        constexpr static const char* state{"state"};
    };

    class Package
    {
    public:
        constexpr static const char* NAME{"click-scope"};

        Package();
        Package(const QSharedPointer<click::web::Client>& client);
        virtual ~Package();

        virtual bool refund(const std::string& pkg_name);
        virtual bool verify(const std::string& pkg_name);
        virtual click::web::Cancellable get_purchases(std::function<void(const PurchaseSet& purchased_apps)> callback);
        static std::string get_base_url();
        static Package& instance();

    protected:
        virtual void setup_pay_service();
        virtual void pay_package_refund(const std::string& pkg_name);
        virtual void pay_package_verify(const std::string& pkg_name);

        struct Private;
        QScopedPointer<pay::Package::Private> impl;

        bool running = false;
        QSharedPointer<click::web::Client> client;
    public:
        std::map<std::string, StatusFunction> callbacks;
    };

} //namespace pay

#endif // _PAY_H_
