/*
 * Copyright 2014 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of version 3 of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include <QDir>
#include <QList>
#include <QTest>
#include <QSharedPointer>
#include <iostream>

class TestRunner {
 public:

    static TestRunner& Instance() {
        static TestRunner instance;
        return instance;
    }

    template <typename T>
    char RegisterTest(char* name) {
        if(!_tests.contains(name)) {
            QSharedPointer<QObject> test(new T());
            _tests.insert(name, QSharedPointer<QObject>(test));
        }
        return char(1);
    }

    int RunAll(int argc, char *argv[]) {
        // provide command line to run a single test case
        QCoreApplication* app = QCoreApplication::instance();
        QStringList args = app->arguments();

        if (args.contains("-testcase")) {
            int index = args.indexOf("-testcase");
            if (args.count() > index + 1) {
                QString testcase = args[index + 1];
                if (_tests.contains(testcase)) {
                    args.removeAt(index + 1);
                    args.removeAt(index);
                    return QTest::qExec(_tests[testcase].data(), args);
                } else {
                    return -1;
                }
            } else {
                return -1;
            }
        } else {
            int errorCode = 0;
            foreach (QString const &testName, _tests.keys()) {
                errorCode |= QTest::qExec(_tests[testName].data(), argc, argv);
                std::cout << std::endl;
            }
        return errorCode;
        }
    }

 private:
    QMap<QString, QSharedPointer<QObject> > _tests;
};


// Use this macro after your test declaration
#define DECLARE_TEST(className)\
    static char test_##className = TestRunner::Instance().RegisterTest<className>(const_cast<char *>(#className));

// Use this macro to execute all tests
#define RUN_ALL_QTESTS(argc, argv)\
    TestRunner::Instance().RunAll(argc, argv);

#endif  // TEST_RUNNER_H
