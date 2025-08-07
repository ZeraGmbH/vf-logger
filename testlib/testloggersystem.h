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

    void setupServer(int entityCount = 2,
                     int componentCount = 2,
                     QList<int> entitiesWithAllComponentsStoredAlways = QList<int>());
    TestVeinServer* getServer();
    QMap<int, QList<QString>> getComponentsCreated();
    void appendCustomerDataSystem();
    void appendEventSystem(VeinEvent::EventSystem *system);
    void cleanup();

    void setComponent(int entityId, QString componentName, QVariant newValue);
    void setCustomerDataComponent(QString componentName, QVariant newValue);
    void setComponentValues(int valuesEmittedPerComponent);

    QVariant getValueOfComponent(int entityId, QString componentName);

    void loadDatabase();
    void startLogging(QString sessionName = "DbTestSession1", QString transactionName = "TestTransaction");
    void stopLogging();

    void setNextValueWriteCount(int newValueWriteCount);

    void changeSession(const QString &sessionPath = "test-session2.json", int baseEntityId = 20);
    static QString getCustomerDataPath();

    VeinLogger::DatabaseLogger* getDbLogger();
    TestDbAddSignaller* getSignaller();
    VeinStorage::AbstractEventSystem* getStorage();
    QByteArray dumpStorage(QList<int> entities = QList<int>() << dataLoggerEntityId);

    static const QLatin1String DBNameOpenOk;
    static const QLatin1String DBNameOpenErrorEarly;
    static const QLatin1String DBNameOpenErrorLate;

private:
    DbType m_dbType;
    std::unique_ptr<TestDbAddSignaller> m_testSignaller;
    std::unique_ptr<TestVeinServer> m_server;
    std::unique_ptr<VeinLogger::DatabaseLogger> m_dataLoggerSystem;
    std::unique_ptr<CustomerDataSystem> m_customerDataSystem;
    int m_emitCountTotal = 0;
};

#endif // TESTLOGGERSYSTEM_H
