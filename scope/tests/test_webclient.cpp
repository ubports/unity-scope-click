#include <QtTest/QtTest>
#include "test_webclient.h"

#ifdef USE_FAKES

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


#else // not USE_FAKES

const QString SEARCH_BASE_URL = "https://search.apps.ubuntu.com/";
const QString SEARCH_PATH = "api/v1/search";
const QString SUPPORTED_FRAMEWORKS = "framework:ubuntu-sdk-13.10";
const QString ARCHITECTURE = "architecture:";
const QString DETAILS_PATH = "api/v1/package/%s";

class IntegrationTest : public QCoreApplication
{
    //Q_OBJECT
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
        WebResponse* wr = ws.call(SEARCH_PATH, params);

        connect(wr, &WebResponse::finished, this, &IntegrationTest::gotResults);
        return exec();
    }
    IntegrationTest(int argc, char**argv) : QCoreApplication(argc, argv) {}
    ~IntegrationTest() {}
};

int main(int argc, char**argv)
{
    IntegrationTest app(argc, argv);
    return app.run();
}

#endif // USE_FAKES

