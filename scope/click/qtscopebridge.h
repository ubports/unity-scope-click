#ifndef QTSCOPEQUERY_H
#define QTSCOPEQUERY_H

#include<QEvent>
#include<QObject>
#include<QString>
#include <unity/scopes/ReplyProxyFwd.h>

class QNetworkReply;

/**
 * A class used to inject a Unity Scope query into the
 * Qt infrastructure.
 */

class QtScopeBridge : public QObject {
    Q_OBJECT

public:
    static QEvent::Type startQueryEventType();

    QtScopeBridge(QString query, unity::scopes::SearchReplyProxy proxy);
    ~QtScopeBridge();

    bool event(QEvent *e);

public slots:

    void downloadFinished(QNetworkReply *reply);

private:
    QString query;
    QString queryUri;
    QString architecture;
    unity::scopes::SearchReplyProxy proxy;
    bool started;
};


#endif
