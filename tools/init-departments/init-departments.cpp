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

#include <click/interface.h>
#include <click/key_file_locator.h>
#include <click/index.h>
#include <click/webclient.h>
#include <click/network_access_manager.h>
#include <click/qtbridge.h>
#include <click/departments-db.h>
#include <future>
#include <iostream>
#include <QDebug>
#include <QtGlobal>

enum
{
    // invalid arguments
    DEPTS_ERROR_ARG = 1,

    // network request errors
    DEPTS_ERROR_NETWORK = 2,

    // sqlite db errors
    DEPTS_ERROR_DB = 5,

    // click errors
    DEPTS_ERROR_CLICK_PARSE = 10,
    DEPTS_ERROR_CLICK_CALL = 11,
    DEPTS_ERROR_CLICK_UNKNOWN = 12
};

QSharedPointer<click::KeyFileLocator> keyFileLocator(new click::KeyFileLocator());
click::Interface iface(keyFileLocator);

void noDebug(QtMsgType, const QMessageLogContext&, const QString&) {}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " DBFILE LOCALE1 [LOCALE2 ...]" << std::endl;
        return DEPTS_ERROR_ARG;
    }

    const std::string dbfile(argv[1]);
    const std::set<std::string> locales(argv + 2, argv + argc);

    if (!getenv("INIT_DEPARTMENTS_DEBUG"))
        qInstallMessageHandler(noDebug);

    std::unique_ptr<click::DepartmentsDb> db;
    try
    {
        db.reset(new click::DepartmentsDb(dbfile));
    }
    catch (const std::exception &e)
    {
        std::cerr << "Failed to open departments database " << dbfile << std::endl;
        return DEPTS_ERROR_DB;
    }

    auto nam = QSharedPointer<click::network::AccessManager>(new click::network::AccessManager());
    auto client = QSharedPointer<click::web::Client>(new click::web::Client(nam));
    click::Index index(client);
    std::vector<click::web::Cancellable> cnc;

    std::promise<void> qt_ready;
    std::promise<void> bootstrap_ready;
    std::promise<click::PackageSet> pkgs_ready;
    auto qt_ready_ft = qt_ready.get_future();
    auto bootstrap_ft = bootstrap_ready.get_future();
    auto pkgs_ft = pkgs_ready.get_future();

    std::atomic<int> return_val(0);
    std::atomic<std::vector<std::string>::size_type> num_of_locales(locales.size());
    std::atomic<click::PackageSet::size_type> num_of_pkgs(0);

    //
    // a thread that does bootstrap request
    std::thread net_thread([&]() {
        qt_ready_ft.get();

        for (auto const locale: locales)
        {
            qt::core::world::enter_with_task([&return_val, &bootstrap_ready, &index, &cnc, &num_of_locales, &db, locale]() {

                std::cout << "Getting departments for locale '" << locale << "'" << std::endl;
                cnc.push_back(index.bootstrap([&return_val, &bootstrap_ready, &num_of_locales, &db, locale](const click::DepartmentList& depts, const click::HighlightList&, click::Index::Error error, int) {
                    std::cout << "Bootstrap call for locale '" << locale << "' finished" << std::endl;

                    if (error == click::Index::Error::NoError)
                    {
                        try
                        {
                            db->store_departments(depts, locale);
                            std::cout << "Stored departments for locale '" << locale << "'" << std::endl;
                        }
                        catch (const std::exception& e)
                        {
                            std::cerr << "Failed to update departments database: " << e.what() << std::endl;
                            return_val = DEPTS_ERROR_DB;
                        }

                        if (--num_of_locales == 0)
                        {
                            std::cout << "All locales processed" << std::endl;
                            bootstrap_ready.set_value();
                        }
                    }
                    else
                    {
                        std::cerr << "Network error" << std::endl;
                        return_val = DEPTS_ERROR_NETWORK;
                        bootstrap_ready.set_value();
                        qt::core::world::destroy();
                        return;
                    }
                }));
            });
        }
    });

    //
    // a thread that iterates over all packages and performs details requests
    // it initially blocks on bootstrap_ft and pkgs_ft future
    // (waits until bootstrap finished and main thread gets all the click packages)
    std::thread details_thread([&]() {
        bootstrap_ft.get();
        auto const pkgs = pkgs_ft.get();

        num_of_pkgs = pkgs.size();

        // if click list failed or nothing to do, then end this thread and stop Qt bridge
        if (return_val != 0 || num_of_pkgs == 0)
        {
            std::cerr << "No packages to process or an error occurred, not fetching departments" << std::endl;
            qt::core::world::destroy();
            return;
        }

            std::cout << "Getting package details for " << num_of_pkgs << " packages" << std::endl;

            //
            // note: this queues requests for all the packages; the number of packages to process
            // is kept in num_of_pkgs which gets decreased, the last processed package will stop Qt bridge.
            for (auto const& pkg: pkgs)
            {
                auto const pkgname = pkg.name;

                qt::core::world::enter_with_task([&return_val, &index, &cnc, &num_of_pkgs, pkgname, &db]() {

                    cnc.push_back(index.get_details(pkgname, [&return_val, &num_of_pkgs, pkgname, &db](const click::PackageDetails& details, click::Index::Error error) {
                        std::cout << "Details call for " << pkgname << " finished" << std::endl;

                        if (error == click::Index::Error::NoError)
                        {
                            try
                            {
                                std::cout << "Storing package department for " << pkgname << ", " << details.department << std::endl;
                                db->store_package_mapping(pkgname, details.department);
                            }
                            catch (const std::exception& e)
                            {
                                std::cerr << "Failed to update departments database: " << e.what() << std::endl;
                                return_val = DEPTS_ERROR_DB;
                            }
                        }
                        else
                        {
                            std::cerr << "Network error" << std::endl;
                            return_val = DEPTS_ERROR_NETWORK;
                        }
                        if (--num_of_pkgs == 0)
                        {
                            std::cout << "All packages processed" << std::endl;
                            qt::core::world::destroy();
                        }
                    }));
                });
            }
    });

    //
    // enter Qt world; this blocks until qt::core:;world::destroy() gets called
    qt::core::world::build_and_run(argc, argv, [&pkgs_ready, &qt_ready, &return_val]() {

        qt::core::world::enter_with_task([&return_val, &pkgs_ready, &qt_ready]() {
            std::cout << "Querying click for installed packages" << std::endl;
            iface.get_installed_packages([&return_val, &pkgs_ready, &qt_ready](click::PackageSet pkgs, click::InterfaceError error) {
                if (error == click::InterfaceError::NoError)
                {
                    std::cout << "Found: " << pkgs.size() << " click packages" << std::endl;
                }
                else
                {
                    if (error == click::InterfaceError::ParseError)
                    {
                        std::cerr << "Error parsing click output" << std::endl;
                        return_val = DEPTS_ERROR_CLICK_PARSE;
                    }
                    else if (error == click::InterfaceError::CallError)
                    {
                        std::cerr << "Error calling click command" << std::endl;
                        return_val = DEPTS_ERROR_CLICK_CALL;
                    }
                    else
                    {
                        std::cerr << "An unknown click error occured" << std::endl;
                        return_val = DEPTS_ERROR_CLICK_UNKNOWN;
                    }
                }
                qt_ready.set_value();
                pkgs_ready.set_value(pkgs); // this unblocks net_thread
            });
        });
    });

    net_thread.join();
    details_thread.join();

    std::cout << std::endl << "Summary:" << std::endl
        << "Number of department mappings: " << db->department_mapping_count() << std::endl
        << "Number of department names (all locales): " << db->department_name_count() << std::endl
        << "Number of applications: " << db->package_count() << std::endl;

    return return_val;
}
