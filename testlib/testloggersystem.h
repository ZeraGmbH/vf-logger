#ifndef TESTLOGGERSYSTEM_H
#define TESTLOGGERSYSTEM_H

#include "customerdatasystem.h"
#include "testdbaddsignaller.h"
#include "vl_databaselogger.h"
#include <testveinserver.h>
#include <QObject>
#include <memory>

static int constexpr systemEntityId = 0;
static int constexpr dataLoggerEntityId = 2;
static int constexpr customerDataEntityId = 200;

class TestLoggerSystem
{
public:
    enum DbType { MOCK, SQLITE };
    TestLoggerSystem(DbType dbType = MOCK);

    void setupServer(int entityCount=2, int componentCount=2);
    QMap<int, QList<QString>> getComponentsCreated();
    void appendCustomerDataSystem();
    void cleanup();

    void setComponent(int entityId, QString componentName, QVariant newValue);
    void setComponentValues(int valuesEmittedPerComponent);

    void loadDatabase();
    void startLogging(QString sessionName = "DbTestSession1", QString transactionName = "TestTransaction");

    void changeSession(const QString &sessionPath = "test-session2.json", int baseEntityId = 20);
    static QString getCustomerDataPath();

    TestDbAddSignaller* getSignaller();
    QByteArray dumpStorage(QList<int> entities = QList<int>() << dataLoggerEntityId);

private:
    DbType m_dbType;
    VeinLogger::AbstractLoggerDB *m_db = nullptr;
    std::unique_ptr<TestDbAddSignaller> m_testSignaller;
    std::unique_ptr<TestVeinServer> m_server;
    VeinEvent::StorageSystem* m_storage;
    std::unique_ptr<VeinLogger::DatabaseLogger> m_dataLoggerSystem;
    std::unique_ptr<CustomerDataSystem> m_customerDataSystem;
};

#endif // TESTLOGGERSYSTEM_H
