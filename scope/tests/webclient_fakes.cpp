#include <QTimer>

#include <webclient_fakes.h>

#ifdef USE_FAKES

QList<QByteArray> FakeNam::scripted_responses;
QList<QNetworkRequest> FakeNam::performed_requests;

FakeReply* FakeNam::get(QNetworkRequest &request)
{
    FakeReply* reply = new FakeReply();
    performed_requests.append(request);
    QTimer::singleShot(0, reply, SLOT(sendFinished()));
    return reply;
}

void FakeReply::sendFinished()
{
    emit finished();
}

QByteArray FakeReply::readAll()
{
    return QByteArray(FakeNam::scripted_responses.takeFirst());
}

#endif // USE_FAKES
