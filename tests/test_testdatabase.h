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

    void recordVeinDump();
    void recordOneContentSet();
    void recordTwoContentSets();
    void recordAllContentSets();
    void recordStartStop();

    void getAllSessions();
    void getNoSession();

    // corner cases
    void noRecordTransactionMissing();
    void noRecordSessionMissing();
    void noRecordDatbaseMissing();
    void removeDbFileForUsbStickGone();
    void openRunLogAndClose(); // TODO bugs
    void guiContextMakesItIntoDbAndVein();
    // TODO: Stop recording on session change

private:
    void removeTimeInfoInTransactions(QJsonObject &sessionInfo);
    TestLoggerSystem m_testSystem;
};

#endif // TEST_TESTDATABASE_H
