#include <QtTest/QtTest>
#include "test_webclient.h"

void TestWebClient::init()
{
    FakeNam::scripted_responses.clear();
    FakeNam::performed_requests.clear();
    results = "";
}

void TestWebClient::testUrlBuiltNoParams()
{
    FakeNam::scripted_responses.append("HOLA");
    WebService ws(FAKE_SERVER);
    ws.call(FAKE_PATH);
    QTRY_VERIFY(FakeNam::scripted_responses.empty());
    QCOMPARE(FakeNam::performed_requests.at(0).url().toString(),
             QString("http://fake-server/fake/api/path"));
}

void TestWebClient::testParamsAppended()
{
    FakeNam::scripted_responses.append("HOLA");
    WebService ws(FAKE_SERVER);
    WebCallParams params;
    params.add("a", "1");
    params.add("b", "2");
    ws.call(FAKE_PATH, params);
    QTRY_VERIFY(FakeNam::scripted_responses.empty());
    QCOMPARE(FakeNam::performed_requests.at(0).url().toString(),
             QString("http://fake-server/fake/api/path?a=1&b=2"));
}

void TestWebClient::testResultsAreEmmited()
{
    FakeNam::scripted_responses.append("HOLA");
    WebService ws(FAKE_SERVER);
    WebResponse* wr = ws.call(FAKE_PATH);
    connect(wr, &WebResponse::finished, this, &TestWebClient::gotResults);
    QTRY_COMPARE(results, QString("HOLA"));
}
