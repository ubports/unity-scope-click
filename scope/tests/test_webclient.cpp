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
#include "test_webclient.h"

void TestWebClient::init()
{
    FakeNam::scripted_responses.clear();
    FakeNam::performed_get_requests.clear();
    results = "";
}

void TestWebClient::testUrlBuiltNoParams()
{
    FakeNam::scripted_responses.append("HOLA");
    WebService ws(FAKE_SERVER);
    QScopedPointer<WebResponse> wr(ws.call(FAKE_PATH));
    QTRY_VERIFY(FakeNam::scripted_responses.empty());
    QVERIFY(!FakeNam::performed_get_requests.empty());
    QCOMPARE(FakeNam::performed_get_requests.at(0).url().toString(),
             QString("http://fake-server/fake/api/path"));
}

void TestWebClient::testParamsAppended()
{
    FakeNam::scripted_responses.append("HOLA");
    WebService ws(FAKE_SERVER);
    WebCallParams params;
    params.add("a", "1");
    params.add("b", "2");
    QScopedPointer<WebResponse> wr(ws.call(FAKE_PATH, params));
    QTRY_VERIFY(FakeNam::scripted_responses.empty());
    QVERIFY(!FakeNam::performed_get_requests.empty());
    QCOMPARE(FakeNam::performed_get_requests.at(0).url().toString(),
             QString("http://fake-server/fake/api/path?a=1&b=2"));
}

void TestWebClient::testResultsAreEmmited()
{
    FakeNam::scripted_responses.append("HOLA");
    WebService ws(FAKE_SERVER);
    QScopedPointer<WebResponse> wr(ws.call(FAKE_PATH));
    connect(wr.data(), &WebResponse::finished, this, &TestWebClient::gotResults);
    QTRY_COMPARE(results, QString("HOLA"));
}
