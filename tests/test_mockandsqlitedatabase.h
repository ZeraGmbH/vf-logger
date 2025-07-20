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
    void displaySessionInfoBeforeDbLoaded();

    void deleteTransaction();
    void deleteNonexistingTransaction();
    void deleteTransactionBeforeDbLoaded();
    void deleteSession();
    void deleteNonexistingSession();
    void deleteSessionBeforeDbLoaded();

    void listAllSessions();
    void listNoSession();
    void listAllSessionsBeforeDbLoaded();

    void displayLoggedValues();
    void displayLoggedValuesOnDifferentSessionDeviceName();
    void displayLoggedValuesZeraAll();
    void displayLoggedValuesInvalidTransaction();
    void displayLoggedValuesBeforeDbLoaded();

    void taskAddTransaction();

private:
    void logATransaction(QString session, QString transaction, QStringList contentSets);
    QVariant getResultCode(QVariant rpcReturnData);
    QString getErrorString(QVariant rpcReturnData);
    QVariant getReturnedResult(QVariant rpcReturnData);
    void removeTimeInfoInTransactions(QJsonObject &sessionInfo);
    QVariant getComponentValue(int entityId, QString component);
    QJsonObject filterEntitySystem(QJsonObject obj);
    std::unique_ptr<TestLoggerSystem> m_testSystem;
};

#endif // TEST_MOCKANDSQLITEDATABASE_H
