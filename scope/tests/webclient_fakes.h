#ifndef WEBCALL_FAKES_H
#define WEBCALL_FAKES_H

#ifdef USE_FAKES

#include <QObject>
#include <QNetworkReply>

class FakeReply : public QObject
{
    Q_OBJECT
public slots:
    void sendFinished();
signals:
    void finished();
public:
    QByteArray readAll();
};

class FakeNam : public QObject
{
    Q_OBJECT
public:
    FakeReply* get(QNetworkRequest& request);
    static QList<QByteArray> scripted_responses;
    static QList<QNetworkRequest> performed_requests;
};

#define QNetworkAccessManager FakeNam
#define QNetworkReply FakeReply

#endif // USE_FAKES
#endif // WEBCALL_FAKES_H
