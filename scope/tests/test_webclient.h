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

#ifndef _TEST_WEBCLIENT_H_
#define _TEST_WEBCLIENT_H_

#include "test_runner.h"
#include "webclient.h"

const QString FAKE_SERVER = "http://fake-server/";
const QString FAKE_PATH = "fake/api/path";
const QString FAKE_JSON_SEARCH_RESULT = " [ { \"name\": \"org.example.awesomelauncher\", \"title\": \"Awesome Launcher\", \"description\": \"This is an awesome launcher.\", \"price\": 1.99, \"icon_url\": \"http://software-center.ubuntu.com/site_media/appmedia/2012/09/SPAZ.png\", \"resource_url\": \"http://search.apps.ubuntu.com/api/v1/package/org.example.awesomelauncher\" }, { \"name\": \"org.example.fantastiqueapp\", \"title\": \"Fantastic App\", \"description\": \"This is a fantasticc app.\", \"price\": 0.0, \"icon_url\": \"http://assets.ubuntu.com/sites/ubuntu/504/u/img/ubuntu/features/icon-find-more-apps-64x64.png\", \"resource_url\": \"http://search.apps.ubuntu.com/api/v1/package/org.example.fantasticapp\" }, { \"name\": \"org.example.awesomewidget\", \"title\": \"Awesome Widget\", \"description\": \"This is an awesome widget.\", \"price\": 1.99, \"icon_url\": \"http://assets.ubuntu.com/sites/ubuntu/504/u/img/ubuntu/features/icon-photos-and-videos-64x64.png\", \"resource_url\": \"http://search.apps.ubuntu.com/api/v1/package/org.example.awesomewidget\" } ] ";

class TestWebClient : public QObject
{
    Q_OBJECT
    QString results;
private slots:
    void init();
    void testUrlBuiltNoParams();
    void testParamsAppended();
    void testResultsAreEmmited();
    void gotResults(QString results)
    {
        this->results = results;
    }
};

DECLARE_TEST(TestWebClient)

#endif /* _TEST_WEBCLIENT_H_ */
