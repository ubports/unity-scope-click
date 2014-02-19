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
#include "key_file_locator.h"
#include "interface.h"

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
#include<vector>
#include<set>

namespace
{



std::string CATEGORY_APPS_DISPLAY = R"(
    {
        "schema-version" : 1,
        "template" : {
            "category-layout" : "grid",
            "card-size": "small"
        },
        "components" : {
            "title" : "title",
            "art" : {
                "field": "art",
                "aspect-ratio": 1.6,
                "fill-mode": "fit"
            }
        }
    }
)";

std::string CATEGORY_APPS_SEARCH = R"(
    {
        "schema-version" : 1,
        "template" : {
            "category-layout" : "journal",
            "card-layout" : "horizontal",
            "card-size": "large"
        },
        "components" : {
            "title" : "title",
            "mascot" : {
                "field": "art"
            },
            "subtitle": "publisher"
        }
    }
)";

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
    ReplyWrapper(QNetworkReply* reply,
                 const scopes::SearchReplyProxy& replyProxy,
                 const std::set<std::string>& installedApplications,
                 const QString& queryUri,
                 QObject* parent)
        : QObject(parent),
          reply(reply),
          replyProxy(replyProxy),
          installedApplications(installedApplications),
          queryUrl(queryUri) {
    }

    void cancel()
    {
        reply->abort();
    }

public slots:
    void downloadFinished(QNetworkReply *reply) {
        // If the user types a search term, multiple queries are kicked off
        // and we have to make sure that we only consider results from the query
        // that this reply corresponds to.
        if (reply->url() != queryUrl)
            return;

        struct Scope
        {
            Scope(QNetworkReply* reply, ReplyWrapper* wrapper)
                : reply(reply),
                  wrapper(wrapper)
            {
            }

            ~Scope()
            {
                reply->deleteLater();
                wrapper->deleteLater();
            }

            QNetworkReply* reply;
            ReplyWrapper* wrapper;
        } scope(reply, this);

        if (reply->error() != QNetworkReply::NoError)
        {
            std::cerr << __PRETTY_FUNCTION__ << ": Received network reply with error: "
                      << reply->errorString().toStdString() << std::endl;
            return;
        }

        try
        {
            QByteArray msg = reply->readAll();
            QJsonParseError jsonParseError;
            QJsonDocument doc = QJsonDocument::fromJson(msg, &jsonParseError);

            if (jsonParseError.error != QJsonParseError::NoError)
                throw std::runtime_error(
                            "ReplyWrapper::onDownloadFinished: Cannot parse JSON from server response with " +
                            jsonParseError.errorString().toStdString());

            QJsonArray results = doc.array();

            for(const auto &entry : results) {
                if(not entry.isObject()) {
                    // FIXME: write message about invalid JSON here.
                    continue;
                }
                scopes::CategorisedResult res(category);
                QJsonObject obj = entry.toObject();
                std::string resourceUrl = obj[click::Query::JsonKeys::RESOURCE_URL].toString().toUtf8().data();
                std::string title = obj[click::Query::JsonKeys::TITLE].toString().toUtf8().data();
                std::string iconUrl = obj[click::Query::JsonKeys::ICON_URL].toString().toUtf8().data();
                std::string name = obj[click::Query::JsonKeys::NAME].toString().toUtf8().data();

                if (installedApplications.count(title) > 0)
                    continue;

                res.set_uri(queryUrl.toString().toUtf8().data());
                res.set_title(title);
                res.set_art(iconUrl);
                res.set_dnd_uri(queryUrl.toString().toUtf8().data());
                res[click::Query::ResultKeys::NAME] = name;
                res[click::Query::ResultKeys::INSTALLED] = false;
                res[click::Query::ResultKeys::DOWNLOAD_URL] = resourceUrl;
                // FIXME at this point we should go through the rest of the fields
                // and convert them.
                replyProxy->push(res);
            }
        } catch(const std::exception& e)
        {
            std::cerr << __PRETTY_FUNCTION__ << ": Caught std::exception with: " << e.what() << std::endl;
        } catch(...)
        {
            std::cerr << __PRETTY_FUNCTION__ << ": Caught exception" << std::endl;
        }
    }

    void setCategoryTemplate(std::string categoryTemplate) {
        scopes::CategoryRenderer categoryRenderer(categoryTemplate);
        category = scopes::Category::SCPtr(replyProxy->register_category("appstore", "Available", "", categoryRenderer));
    }

private:
    QNetworkReply* reply;
    scopes::SearchReplyProxy replyProxy;
    std::set<std::string> installedApplications;
    scopes::Category::SCPtr category;
    QUrl queryUrl;
};
}

static void push_local_results(scopes::SearchReplyProxy const &replyProxy,
                               std::vector<click::Application> const &apps,
                               std::string categoryTemplate)
{
    scopes::CategoryRenderer rdr(categoryTemplate);
    auto cat = replyProxy->register_category("local", "My apps", "", rdr);

    for(const auto & a: apps) 
    {
        scopes::CategorisedResult res(cat);
        res.set_title(a.title);
        res.set_art(a.icon_url);
        res.set_uri(a.url);
        res[click::Query::ResultKeys::NAME] = a.name;
        res[click::Query::ResultKeys::DESCRIPTION] = a.description;
        res[click::Query::ResultKeys::MAIN_SCREENSHOT] = a.main_screenshot;
        res[click::Query::ResultKeys::INSTALLED] = true;
        replyProxy->push(res);
    }
}

struct click::Query::Private
{
    Private(const std::string& query)
        : query(query)
    {
    }
    std::string query;
    qt::HeapAllocatedObject<ReplyWrapper> replyWrapper;
};

click::Query::Query(std::string const& query)
    : impl(new Private(query))
{
}

click::Query::~Query()
{
}

void click::Query::cancelled()
{
    qt::core::world::enter_with_task([this](qt::core::world::Environment& env)
    {
        env.resolve(impl->replyWrapper)->cancel();
    });
}

namespace
{
click::Interface& clickInterfaceInstance()
{
    static QSharedPointer<click::KeyFileLocator> keyFileLocator(new click::KeyFileLocator());
    static click::Interface iface(keyFileLocator);

    return iface;
}
}
void click::Query::run(scopes::SearchReplyProxy const& searchReply)
{
    QString query = QString::fromStdString(impl->query);
    std::string categoryTemplate = CATEGORY_APPS_SEARCH;
    if (query.isEmpty()) {
        categoryTemplate = CATEGORY_APPS_DISPLAY;
    }
    auto localResults = clickInterfaceInstance().find_installed_apps(
                query);

    push_local_results(
        searchReply, 
        localResults,
        categoryTemplate);

    std::set<std::string> locallyInstalledApps;
    for(const auto& app : localResults)
        locallyInstalledApps.insert(app.title);

    qt::core::world::enter_with_task([this, searchReply, locallyInstalledApps, categoryTemplate](qt::core::world::Environment& env)
    {
        static const QString queryPattern(
                    "https://search.apps.ubuntu.com/api/v1/search?q=%1"
                    ",framework:ubuntu-sdk-13.10,architecture:%2");
        QString queryUri = queryPattern.arg(QString::fromUtf8(impl->query.c_str())).arg(architecture());

        auto nam = getNetworkAccessManager(env);
        auto networkReply = nam->get(QNetworkRequest(QUrl(queryUri)));

        impl->replyWrapper = env.allocate<ReplyWrapper>(networkReply, searchReply, locallyInstalledApps, queryUri, &env);
        auto rw = env.resolve(impl->replyWrapper);
        rw->setCategoryTemplate(categoryTemplate);

        QObject::connect(
                    nam, &QNetworkAccessManager::finished,
                    rw, &ReplyWrapper::downloadFinished);
    });
}

#include "query.moc"
