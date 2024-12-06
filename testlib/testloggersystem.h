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

    void setupServer(int entityCount=2, int componentCount=2, QList<int> entitiesWithAllComponentsStoredAlways = QList<int>());
    QMap<int, QList<QString>> getComponentsCreated();
    void appendCustomerDataSystem();
    void cleanup();

    void setDataComponent(int entityId, QString componentName, QVariant newValue);
    void setControlComponent(int entityId, QString componentName, QVariant newValue);
    void setComponentValuesSequenceEach(int valuesEmittedPerComponent);

    void loadDatabase();
    void startLogging(QString sessionName = "DbTestSession1", QString transactionName = "TestTransaction");
    void stopLogging();
    void setNextValueWriteCount(int newValueWriteCount);

    void changeSession(const QString &sessionPath = "test-session2.json", int baseEntityId = 20);
    static QString getCustomerDataPath();

    TestDbAddSignaller* getSignaller();
    QByteArray dumpStorage(QList<int> entities = QList<int>() << dataLoggerEntityId);

private:
    DbType m_dbType;
    std::unique_ptr<TestDbAddSignaller> m_testSignaller;
    std::unique_ptr<TestVeinServer> m_server;
    std::unique_ptr<VeinLogger::DatabaseLogger> m_dataLoggerSystem;
    std::unique_ptr<CustomerDataSystem> m_customerDataSystem;
};

#endif // TESTLOGGERSYSTEM_H
