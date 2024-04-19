#ifndef TEST_TESTDATABASE_H
#define TEST_TESTDATABASE_H

#include "testloggersystem.h"

class test_testdatabase : public QObject
{
    Q_OBJECT
private slots:
    void cleanup();

    void openDatabaseErrorEarly();
    void openDatabaseErrorLate();
    void openDatabaseOk();

    void createSessionNoCustomerDataSystem();
    void setupCustomerData();
    void selectExistingSession();
    void createSession();
    void createSessionWithCustomerDataAlreadyCreated();

    void recordVeinDump();
    void recordOneContentSet();
    void recordTwoContentSets();
    void recordAllContentSets();

    // corner cases
    void noRecordTransactionMissing();
    void noRecordSessionMissing();
    void removeDbFileForUsbStickGone();
    void openRunLogAndClose(); // TODO bugs
    void guiContextMakesItIntoDbAndVein();

private:
    void loadDatabase();
    void setComponentValues(int valuesEmittedPerComponent);
    void startLogging();

    TestLoggerSystem m_testSystem;
};

#endif // TEST_TESTDATABASE_H
