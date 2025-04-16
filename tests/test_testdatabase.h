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

    void displaySessionInfos();
    void displaySessionInfosInvalidSession();
    void displaySessionInfosMultipleTransactions();

    void deleteTransaction();
    void deleteNonexistingTransaction();
    void deleteSession();
    void deleteNonexistingSession();

    // corner cases
    void noRecordTransactionMissing();
    void noRecordSessionMissing();
    void noRecordDatbaseMissing();
    void removeDbFileForUsbStickGone();
    void openRunLogAndClose(); // TODO bugs
    void guiContextMakesItIntoDbAndVein();
    // TODO: Stop recording on session change

private:
    void removeTimeInfo(QJsonObject &sessionInfo, QString transaction);
    TestLoggerSystem m_testSystem;
};

#endif // TEST_TESTDATABASE_H
