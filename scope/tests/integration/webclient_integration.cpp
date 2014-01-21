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

#include <QtTest/QtTest>
#include "webclient.h"

const QString SEARCH_BASE_URL = "https://search.apps.ubuntu.com/";
const QString SEARCH_PATH = "api/v1/search";
const QString SUPPORTED_FRAMEWORKS = "framework:ubuntu-sdk-13.10";
const QString ARCHITECTURE = "architecture:";
const QString DETAILS_PATH = "api/v1/package/%s";

class IntegrationTest : public QCoreApplication
{
    Q_OBJECT

    QScopedPointer<WebResponse> wr;

    void gotResults(QString results)
    {
        qDebug() << results;
        quit();
    }

public:
    int run()
    {
        WebService ws(SEARCH_BASE_URL);
        WebCallParams params;
        params.add("q", "qr,architecture:armhf");
        wr.reset(ws.call(SEARCH_PATH, params));

        connect(wr.data(), &WebResponse::finished, this, &IntegrationTest::gotResults);
        return exec();
    }
    IntegrationTest(int argc, char**argv) : QCoreApplication(argc, argv) {}
};

int main(int argc, char**argv)
{
    IntegrationTest app(argc, argv);
    return app.run();
}

#include "webclient_integration.moc"
