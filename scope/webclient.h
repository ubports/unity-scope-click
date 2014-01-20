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

#ifndef WEBCALL_H
#define WEBCALL_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

#include "tests/webclient_fakes.h"

class WebService;

class WebCallParams : public QObject
{
    Q_OBJECT
    QUrlQuery query;
    friend class WebService;
public:
    void add(QString key, QString value)
    {
        query.addQueryItem(key, value);
    }
};

class WebResponse : public QObject
{
    Q_OBJECT
    QNetworkReply* reply;
public:
    WebResponse(QNetworkReply *_reply);
private slots:
    void replyFinished();
signals:
    void finished(QString result);

};

class WebService : public QObject
{
    Q_OBJECT
    QString base_url;
    static QNetworkAccessManager qnam;
public:
    WebService(QString base) : base_url(base)
    {
    }
    WebResponse* call(QString path, WebCallParams& params);
    WebResponse* call(QString path);
};


#endif // WEBCALL_H
