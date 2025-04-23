#ifndef TEST_DATABASE_H
#define TEST_DATABASE_H

#include "testloggersystem.h"
#include <QObject>

class test_database : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase_data();
    void init();
    void cleanup();

    void displaySessionInfo();
    void displaySessionInfosInvalidSession();
    void displaySessionInfosMultipleTransactions();

    void deleteTransaction();
    void deleteNonexistingTransaction();
    void deleteSession();
    void deleteNonexistingSession();

    void getAllSessions();
    void getNoSession();

private:
    void removeTimeInfoInTransactions(QJsonObject &sessionInfo);
    std::unique_ptr<TestLoggerSystem> m_testSystem;
};

#endif // TEST_DATABASE_H
