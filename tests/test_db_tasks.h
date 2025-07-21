#ifndef TEST_DB_TASKS_H
#define TEST_DB_TASKS_H

#include "testloggersystem.h"
#include <QObject>

class test_db_tasks : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase_data();
    void init();
    void cleanup();

    void taskAddSession();
    void taskAddSessionTwice();
    void taskAddSessionNoDb();
    void taskAddSessionNoSessionName();

    void taskAddTransaction();
    void taskAddTransactionTwice();
    void taskAddTransactionNoDb();
    void taskAddTransactionNoTransactionName();
    void taskAddTransactionNoSessionName();
    void taskAddTransactionNoContentSets();

    void taskUpdateTransactionStartTime();
    void taskUpdateTransactionStartTimeNoDb();
    //void taskUpdateTransactionStartTimeInvalidTaskId(); // TODO: SQLite implementation accepts all transaction ids

    void taskUpdateTransactionStopTime();
    void taskUpdateTransactionStopTimeNoDb();
    //void taskUpdateTransactionStopTimeInvalidTaskId(); // TODO: SQLite implementation accepts all transaction ids

private:
    std::unique_ptr<TestLoggerSystem> m_testSystem;
};

#endif // TEST_DB_TASKS_H
