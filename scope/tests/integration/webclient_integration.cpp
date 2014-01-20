#include <QtTest/QtTest>
#include "webclient.h"

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
