#ifndef TEST_TESTDATABASE_H
#define TEST_TESTDATABASE_H

#include "testloggersystem.h"

class test_testdatabase : public QObject
{
    Q_OBJECT
private slots:
    void cleanup();

    void createSessionNoCustomerDataSystem();
    void setupCustomerData();
    void selectExistingSession();
    void createSession();

    void recordVeinDump();
    void recordOneContentSet();
    void recordTwoContentSets();
    void recordAllContentSets();
    void recordStartStop();

    // corner cases
    void noRecordTransactionMissing();
    void noRecordSessionMissing();
    void noRecordDatbaseMissing();
    void removeDbFileForUsbStickGone();
    void openRunLogAndClose(); // TODO bugs
    void guiContextMakesItIntoDbAndVein();
    // TODO: Stop recording on session change

private:
    TestLoggerSystem m_testSystem;
};

#endif // TEST_TESTDATABASE_H
