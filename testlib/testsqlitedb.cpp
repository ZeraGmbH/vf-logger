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

void TestSQLiteDB::onAddTransaction(const QString &transactionName, const QString &sessionName, const QStringList &contentSets, const QString &guiContextName)
{
    SQLiteDB::onAddTransaction(transactionName, sessionName, contentSets, guiContextName);
    emit m_testSignaller->sigAddTransaction(transactionName, sessionName, contentSets, guiContextName);
}
