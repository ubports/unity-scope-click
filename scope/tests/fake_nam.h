#ifndef _FAKE_NAM_H_
#define _FAKE_NAM_H_

#ifdef USE_FAKE_NAM

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
    FakeReply* post(QNetworkRequest& request, QByteArray& data);
    static QList<QByteArray> scripted_responses;
    static QList<QNetworkRequest> performed_requests;
};

#define QNetworkAccessManager FakeNam
#define QNetworkReply FakeReply

#endif // USE_FAKE_NAM
#endif // _FAKE_NAM_H_
