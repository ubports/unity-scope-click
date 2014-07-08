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

static QSharedPointer<click::KeyFileLocator> keyFileLocator(new click::KeyFileLocator());
static click::Interface iface(keyFileLocator);

int main(int argc, char **argv)
{
    /*if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " DBFILE LOCALE1 LOCALE2 ..." << std::endl;
        return DEPTS_ERROR_ARG;
    }*/

    int return_val = 0;
    auto nam = QSharedPointer<click::network::AccessManager>(new click::network::AccessManager());
    auto client = QSharedPointer<click::web::Client>(new click::web::Client(nam));
    click::Index index(client);
    click::web::Cancellable cnc;

    qt::core::world::build_and_run(argc, argv, [&return_val,&index, &cnc]() {
        qt::core::world::enter_with_task([&return_val, &index, &cnc]() {
            iface.get_installed_packages([&return_val, &index, &cnc](click::PackageSet pkgs, click::InterfaceError error) {
                if (error == click::InterfaceError::NoError)
                {
                    qDebug() << "Found:" << pkgs.size() << "click packages";

                    qDebug() << "Getting departments from network";
                    cnc = index.bootstrap([&return_val](const click::DepartmentList& depts, const click::HighlightList&, click::Index::Error error, int) {
                        qDebug() << "Bootstrap done";

                        if (error == click::Index::Error::NoError)
                        {
                            try
                            {
                                click::DepartmentsDb db("depts.db"); //FIXME: db file
                                db.store_departments(depts, ""); //FIXME: locale
                                qDebug() << "Got" << db.department_name_count() << "departments";
                            }
                            catch (const std::exception& e)
                            {
                                qWarning() << "Failed to update departments database:" << QString::fromStdString(e.what());
                                return_val = DEPTS_ERROR_DB;
                            }
                        }
                        else
                        {
                            qWarning() << "Network error";
                            return_val = DEPTS_ERROR_NETWORK;
                        }

                        qt::core::world::destroy();
                    });
                }
                else
                {
                    if (error == click::InterfaceError::ParseError)
                    {
                        qWarning() << "Error parsing click output";
                        return_val = DEPTS_ERROR_CLICK_PARSE;
                    }
                    else if (error == click::InterfaceError::CallError)
                    {
                        qWarning() << "Error calling click command";
                        return_val = DEPTS_ERROR_CLICK_CALL;
                    }
                    else
                    {
                        qWarning() << "An unknown click error occured";
                        return_val = DEPTS_ERROR_CLICK_UNKNOWN;
                    }

                    qt::core::world::destroy();
                }
            });
        });
    });

    return return_val;
}
