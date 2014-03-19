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

#include "click/smartconnect.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace ::testing;

namespace
{

class FakeObject : public QObject
{
    Q_OBJECT
public:
    explicit FakeObject(QObject* parent=0) : QObject(parent) {}
signals:
    void signal0();
    void signal1(int x);
    void signal2(int x, QString y);
};

class SmartConnectTest : public Test, QObject
{
public:
    MOCK_METHOD0(handler0, void());
    MOCK_METHOD1(handler1, void(int x));
    MOCK_METHOD2(handler2, void(int x, QString y));
};

} // namespace

TEST_F(SmartConnectTest, testSlotNoArgsCalled)
{
    click::utils::SmartConnect sc;
    FakeObject* o = new FakeObject(&sc);
    sc.connect(o, &FakeObject::signal0, [=](){
        handler0();
    });
    EXPECT_CALL(*this, handler0()).Times(1);
    emit o->signal0();
}

TEST_F(SmartConnectTest, testSlotOneArgCalled)
{
    click::utils::SmartConnect sc;
    FakeObject* o = new FakeObject(&sc);
    sc.connect(o, &FakeObject::signal1, [=](int x){
        handler1(x);
    });
    EXPECT_CALL(*this, handler1(42)).Times(1);
    emit o->signal1(42);
}

TEST_F(SmartConnectTest, testSlotTwoArgsCalled)
{
    click::utils::SmartConnect sc;
    FakeObject* o = new FakeObject(&sc);
    sc.connect(o, &FakeObject::signal2, [=](int x, QString y){
        handler2(x, y);
    });
    EXPECT_CALL(*this, handler2(42, QString("Blue"))).Times(1);
    emit o->signal2(42, "Blue");
}

TEST_F(SmartConnectTest, testDisconnectsOnFirstCalled)
{
    click::utils::SmartConnect sc;
    FakeObject* o = new FakeObject(&sc);
    sc.connect(o, &FakeObject::signal2, [=](int x, QString y){
        handler2(x, y);
    });
    sc.connect(o, &FakeObject::signal1, [=](int x){
        handler1(x);
    });
    EXPECT_CALL(*this, handler2(42, QString("Blue"))).Times(1);
    EXPECT_CALL(*this, handler1(_)).Times(0);
    emit o->signal2(42, "Blue");
    emit o->signal0();
    emit o->signal1(83);
    emit o->signal2(83, "Red");
}

class FakeSmartConnect : public click::utils::SmartConnect
{
    Q_OBJECT
public:
    MOCK_METHOD0(cleanup, void());
};


TEST_F(SmartConnectTest, testCleanupCalledOnDisconnect)
{
    FakeSmartConnect fsc;
    FakeObject* o = new FakeObject(&fsc);
    fsc.connect(o, &FakeObject::signal0, [](){});
    EXPECT_CALL(fsc, cleanup()).Times(1);
    emit o->signal0();
}

#include "test_smartconnect.moc"
