#ifndef TESTSQLITEDB_H
#define TESTSQLITEDB_H

#include "vl_sqlitedb.h"
#include "testdbaddsignaller.h"

class TestSQLiteDB : public VeinLogger::SQLiteDB
{
    Q_OBJECT
public:
    TestSQLiteDB(TestDbAddSignaller* testSignaller);
    bool requiresOwnThread() override { return false; }

    void addComponent(const QString &componentName) override;
    void addEntity(int entityId, QString entityName) override;
    void onAddTransaction(const QString &transactionName, const QString &sessionName, const QStringList &contentSets, const QString &guiContextName) override;

private:
    TestDbAddSignaller *m_testSignaller;
};

#endif // TESTSQLITEDB_H
