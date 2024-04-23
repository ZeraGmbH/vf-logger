#ifndef TESTLOGGERSYSTEM_H
#define TESTLOGGERSYSTEM_H

#include "customerdatasystem.h"
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
    TestLoggerSystem();
    void setupServer(int entityCount=2, int componentCount=2);
    void changeSession(const QString &sessionPath = "test-session2.json", int baseEntityId = 20);
    void cleanup();

    void appendCustomerDataSystem();
    static QString getCustomerDataPath();
    void setComponent(int entityId, QString componentName, QVariant newValue);

    QMap<int, QList<QString>> getComponentsCreated();
    QByteArray dumpStorage(QList<int> entities = QList<int>() << dataLoggerEntityId);

private:
    std::unique_ptr<TestVeinServer> m_server;
    VeinEvent::StorageSystem* m_storage;
    std::unique_ptr<VeinLogger::DatabaseLogger> m_dataLoggerSystem;
    std::unique_ptr<CustomerDataSystem> m_customerDataSystem;
};

#endif // TESTLOGGERSYSTEM_H
