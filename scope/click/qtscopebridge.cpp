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
