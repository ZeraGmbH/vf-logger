#ifndef TEST_TESTDATABASE_H
#define TEST_TESTDATABASE_H

#include "testloggersystem.h"

class test_testdatabase : public QObject
{
    Q_OBJECT
private slots:
    void init();
    void cleanup();

    void openDatabaseErrorEarly();
    void openDatabaseErrorLate();
    void openDatabaseOk();
    void setSessionNotExistentInDb();
    void setSessionExistentInDb();
    void setupCustomerData();
    void setSessionNotExistentWithCustomerData();

private:
    TestLoggerSystem m_testSystem;
};

#endif // TEST_TESTDATABASE_H
