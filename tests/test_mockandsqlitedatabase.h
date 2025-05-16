#ifndef TEST_MOCKANDSQLITEDATABASE_H
#define TEST_MOCKANDSQLITEDATABASE_H

#include "testloggersystem.h"
#include <QObject>

class test_mockandsqlitedatabase : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase_data();
    void init();
    void cleanup();

    void createSessionInsertsEntityComponents();
    void createSessionWithCustomerDataAlreadyCreated();
    void logInsertsEntityComponents();

    void openDatabaseErrorEarly();
    void openDatabaseOk();
    void openDatabaseErrorLate();

    void displaySessionInfo();
    void displaySessionInfosInvalidSession();
    void displaySessionInfosMultipleTransactions();

    void deleteTransaction();
    void deleteNonexistingTransaction();
    void deleteSession();
    void deleteNonexistingSession();

    void getAllSessions();
    void getNoSession();

    void displayLoggedValues();
    void displayLoggedValuesZeraAll();
    void displayLoggedValuesInvalidTransaction();

private:
    void removeTimeInfoInTransactions(QJsonObject &sessionInfo);
    QVariant getComponentValue(int entityId, QString component);
    std::unique_ptr<TestLoggerSystem> m_testSystem;
};

#endif // TEST_MOCKANDSQLITEDATABASE_H
