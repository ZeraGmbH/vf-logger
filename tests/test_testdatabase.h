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

    void createSessionNoCustomerDataSystem();

    // from here all have customer data system
    void setupCustomerData();
    void selectExistingSession();
    void createSession();
    void createSessionWithCustomerDataAlreadyCreated();

    void removeDbFileForUsbStickGone();
    void openSelectExistingSessionAndClose(); // TODO bugs

    void recordVeinDump();
    void recordOneContentSet();
private:
    void startLoggerWithComponents();
    void setInitialVeinComponents();
    void setLoggerOnComponents();
    TestLoggerSystem m_testSystem;
};

#endif // TEST_TESTDATABASE_H
