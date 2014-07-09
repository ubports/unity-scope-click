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
    if (getenv("INIT_DEPARTMENTS_DEBUG") == nullptr)
    {
        qInstallMessageHandler(noDebug);
    }

    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " DBFILE LOCALE" << std::endl;
        return DEPTS_ERROR_ARG;
    }

    int return_val = 0;
    const std::string dbfile(argv[1]);
    const std::string locale(argv[2]);

    auto nam = QSharedPointer<click::network::AccessManager>(new click::network::AccessManager());
    auto client = QSharedPointer<click::web::Client>(new click::web::Client(nam));
    click::Index index(client);
    click::web::Cancellable cnc;

    std::promise<void> qt_ready;
    std::promise<void> bootstrap_ready;

    auto qt_ready_ft = qt_ready.get_future();
    auto bootstrap_ft = bootstrap_ready.get_future();

    std::thread thr([&]() {
        qt_ready_ft.get();

        qt::core::world::enter_with_task([&return_val, &bootstrap_ready, &index, &cnc, dbfile, locale]() {

            std::cout << "Getting departments for locale " << locale << std::endl;
            cnc = index.bootstrap([&return_val, &bootstrap_ready, dbfile, locale](const click::DepartmentList& depts, const click::HighlightList&, click::Index::Error error, int) {
                std::cout << "Bootstrap call finished" << std::endl;

                if (error == click::Index::Error::NoError)
                {
                    try
                    {
                        std::cout << "Storing departments in " << dbfile << " database" << std::endl;
                        click::DepartmentsDb db(dbfile);
                        db.store_departments(depts, locale);
                        std::cout << "Stored " << db.department_name_count() << " departments" << std::endl;
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

                bootstrap_ready.set_value();
            });
        });
    });

    std::thread thr2([&]() {
            bootstrap_ft.get();
            qt::core::world::destroy();
    });

    //
    // enter Qt world; this blocks until qt::core:;world::destroy() gets called
    qt::core::world::build_and_run(argc, argv, [&qt_ready, &return_val,&index, dbfile, locale]() {

        qt::core::world::enter_with_task([&return_val, &index, &qt_ready]() {
            std::cout << "Querying click for installed packages" << std::endl;
            iface.get_installed_packages([&return_val, &qt_ready](click::PackageSet pkgs, click::InterfaceError error) {
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
            });
        });
    });

    thr.join();
    thr2.join();

    return return_val;
}
