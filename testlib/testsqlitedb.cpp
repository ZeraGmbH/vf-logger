#include "testsqlitedb.h"

TestSQLiteDB::TestSQLiteDB(TestDbAddSignaller *testSignaller) :
    m_testSignaller(testSignaller)
{
}

void TestSQLiteDB::addComponent(const QString &componentName)
{
    SQLiteDB::addComponent(componentName);
    emit m_testSignaller->sigComponentAdded(componentName);
}

void TestSQLiteDB::addEntity(int entityId, QString entityName)
{
    SQLiteDB::addEntity(entityId, entityName);
    emit m_testSignaller->sigEntityAdded(entityId, entityName);
}

int TestSQLiteDB::addTransaction(const QString &transactionName, const QString &sessionName, const QStringList &contentSets, const QString &guiContextName)
{
    int transactionId = SQLiteDB::addTransaction(transactionName, sessionName, contentSets, guiContextName);
    emit m_testSignaller->sigAddTransaction(transactionName, sessionName, contentSets, guiContextName);
    return transactionId;
}

bool TestSQLiteDB::updateTransactionStartTime(int transactionId, QDateTime time)
{
    bool ok = SQLiteDB::updateTransactionStartTime(transactionId, time);
    emit m_testSignaller->sigTransactionUpdateStart(transactionId, time);
    return ok;
}

bool TestSQLiteDB::updateTransactionStopTime(int transactionId, QDateTime time)
{
    bool ok = SQLiteDB::updateTransactionStopTime(transactionId, time);
    emit m_testSignaller->sigTransactionUpdateStop(transactionId, time);
    return ok;
}
