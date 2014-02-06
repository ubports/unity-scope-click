#include"qtscopebridge.h"

#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/Query.h>
#include <unity/scopes/SearchReply.h>

#include<QNetworkAccessManager>
#include<QNetworkRequest>
#include<QNetworkReply>
#include<QDebug>
#include<QJsonDocument>
#include<QJsonArray>
#include<QJsonObject>
#include<QProcess>

using namespace unity::scopes;

QEvent::Type QtScopeBridge::startQueryEventType()
{
    static QEvent::Type type = static_cast<QEvent::Type>(QEvent::registerEventType());
    return type;
}

QtScopeBridge::QtScopeBridge(QString query, unity::scopes::SearchReplyProxy proxy) :
    query(query), proxy(proxy), started(false) {
    QString program("dpkg");
    QStringList arguments;
    arguments << "--print-architecture";
    QScopedPointer<QProcess> archDetector(new QProcess());
    archDetector->start(program, arguments);
    if(!archDetector->waitForFinished()) {
        throw std::runtime_error("Architecture detection failed.");
    }
    auto output = archDetector->readAllStandardOutput();
    auto ostr = QString::fromUtf8(output);
    ostr.remove('\n');
    architecture = ostr;
}

QtScopeBridge::~QtScopeBridge() {
}

bool QtScopeBridge::event(QEvent *e) {
    if (e->type() != QtScopeBridge::startQueryEventType()) {
        return QObject::event(e);
    }

    if(started) {
        qWarning() << "Tried to start the same scope query twice.\n";
        return false;
    }
    started = true;
    /* At this point we are under Qt control and can do whatever
     * we want with Qt libraries and callbacks. In this simple demo we
     * download a web page and return it.
     */
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(downloadFinished(QNetworkReply*)));

    QString base("https://search.apps.ubuntu.com/api/v1/search?q=");
    QString theRest(",framework:ubuntu-sdk-13.10,architecture:");
    queryUri = base + query + theRest + architecture;
    manager->get(QNetworkRequest(QUrl(queryUri)));
    return true;
}

void QtScopeBridge::downloadFinished(QNetworkReply *reply) {
    CategoryRenderer rdr;
    auto cat = proxy->register_category("click", "Click packages", "", rdr);
    const QString scopeUrlKey("resource_url");
    const QString titleKey("title");
    const QString iconUrlKey("icon_url");
    QByteArray msg = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(msg);
    QJsonArray results = doc.array();
    for(const auto &entry : results) {
        if(not entry.isObject()) {
            // FIXME: write message about invalid JSON here.
            continue;
        }
        CategorisedResult res(cat);
        QJsonObject obj = entry.toObject();
        QString scopeUrl = obj[scopeUrlKey].toString();
        QString title = obj[titleKey].toString();
        QString iconUrl = obj[iconUrlKey].toString();
        res.set_uri(queryUri.toUtf8().data());
        if(reply->error() != QNetworkReply::NoError) {
            res.set_title("Click scope encountered a network error");
        } else {
            res.set_title(title.toUtf8().data());
            res.set_art(iconUrl.toUtf8().data());
            res.set_dnd_uri(queryUri.toUtf8().data());
        }
        // FIXME at this point we should go through the rest of the fields
        // and convert them.
        proxy->push(res);
    }

    // All results are pushed, time to remove ourselves.
    // Destruction of *this object signals the end of the result set.
    deleteLater();

}
