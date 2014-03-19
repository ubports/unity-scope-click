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

#ifndef SMARTCONNECT_H
#define SMARTCONNECT_H

#include <QObject>
#include <QDebug>
#include <QSharedPointer>

namespace click {
namespace utils {

class SmartConnect : public QObject
{
    Q_OBJECT

    QList<QMetaObject::Connection> connections;

    virtual void cleanup();
private slots:
    void disconnectAll();

public:
    explicit SmartConnect(QObject *parent = 0);

    template<typename SenderType, typename SignalType, typename SlotType>
    void connect(SenderType* sender,
                 SignalType signal,
                 SlotType slot)
    {
        connections.append(QObject::connect(sender, signal, slot));
        connections.append(QObject::connect(sender, signal, this, &SmartConnect::disconnectAll));
    }
};

} // namespace utils
} // namespace click

#endif // SMARTCONNECT_H
