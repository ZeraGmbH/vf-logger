#ifndef TEST_TESTLIB_TESTLOGGERSYSTEM_H
#define TEST_TESTLIB_TESTLOGGERSYSTEM_H

#include "testloggersystem.h"

class test_testlib_testloggersystem : public QObject
{
    Q_OBJECT
private slots:
    void init();
    void cleanup();

    void systemSetupProperly();
private:
    TestLoggerSystem m_testSystem;
};

#endif // TEST_TESTLIB_TESTLOGGERSYSTEM_H
