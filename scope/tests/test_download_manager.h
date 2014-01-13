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

#ifndef _TEST_DOWNLOAD_MANAGER_H_
#define _TEST_DOWNLOAD_MANAGER_H_

#include <QObject>
#include <QDebug>
#include <QString>
#include <QTest>
#include <QSignalSpy>

#include "test_runner.h"
#include "download-manager.h"

using namespace ClickScope;

class TestDownloadManager : public QObject
{
    Q_OBJECT

private slots:
    void testFetchClickTokenCredentialsFound();
//    void testFetchClickTokenCredentialsNotFound();
};

DECLARE_TEST(TestDownloadManager)

#endif /* _TEST_DOWNLOAD_MANAGER_H_ */
