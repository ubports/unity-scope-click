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

#include "query.h"
#include "qtbridge.h"

#if UNITY_SCOPES_API_HEADERS_NOW_UNDER_UNITY
#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/Query.h>
#include <unity/scopes/SearchReply.h>
#else
#include <scopes/Annotation.h>
#include <scopes/CategoryRenderer.h>
#include <scopes/CategorisedResult.h>
#include <scopes/Query.h>
#include <scopes/SearchReply.h>
#endif

#include<QJsonDocument>
#include<QJsonArray>
#include<QJsonObject>
#include<QNetworkReply>
#include<QNetworkRequest>
#include<QProcess>
#include<QStringList>
#include<QUrl>

namespace
{
QNetworkAccessManager* getNetworkAccessManager(qt::core::world::Environment& env)
{
    static qt::HeapAllocatedObject<QNetworkAccessManager> nam = env.allocate<QNetworkAccessManager>(&env);
    return env.resolve(nam);
}

QString architectureFromDpkg()
{
    QString program("dpkg");
    QStringList arguments;
    arguments << "--print-architecture";
    QProcess archDetector;
    archDetector.start(program, arguments);
    if(!archDetector.waitForFinished()) {
        throw std::runtime_error("Architecture detection failed.");
    }
    auto output = archDetector.readAllStandardOutput();
    auto ostr = QString::fromUtf8(output);
    ostr.remove('\n');

    return ostr;
}

const QString& architecture()
{
    static const QString arch{architectureFromDpkg()};
    return arch;
}

class ReplyWrapper : public QObject
{
    Q_OBJECT

public:
    ReplyWrapper(const scopes::SearchReplyProxy& replyProxy,
                 const QString& queryUri,
                 QObject* parent)
        : QObject(parent),
          replyProxy(replyProxy),
          queryUri(queryUri) {
    }

    void downloadFinished(QNetworkReply *reply) {
        scopes::CategoryRenderer rdr;
        auto cat = replyProxy->register_category("click", "Click packages", "", rdr);
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
            scopes::CategorisedResult res(cat);
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
            replyProxy->push(res);
        }

        // All results are pushed, time to remove ourselves.
        // Destruction of *this object signals the end of the result set.
        deleteLater();
    }

    scopes::SearchReplyProxy replyProxy;
    QString queryUri;
};
}

click::Query::Query(std::string const& query)
    : query(query)
{
}

click::Query::~Query()
{
}

void click::Query::cancelled()
{
}

void click::Query::run(scopes::SearchReplyProxy const& reply)
{
    qt::core::world::enter_with_task([this, reply](qt::core::world::Environment& env)
    {
        static const QString base("https://search.apps.ubuntu.com/api/v1/search?q=");
        static const QString theRest(",framework:ubuntu-sdk-13.10,architecture:");
        QString queryUri = base + QString::fromUtf8(query.c_str()) + theRest + architecture();
        std::cout << "Executing search in the qt world." << std::endl;
        auto replyWrapper = new ReplyWrapper(reply, queryUri, &env);

        auto nam = getNetworkAccessManager(env);

        QObject::connect(
                    nam, SIGNAL(finished(QNetworkReply*)),
                    replyWrapper, SLOT(downloadFinished(QNetworkReply*)));

        nam->get(QNetworkRequest(QUrl(queryUri)));
    });
}

#include "query.moc"
