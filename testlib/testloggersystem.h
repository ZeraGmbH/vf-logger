#ifndef TESTLOGGERSYSTEM_H
#define TESTLOGGERSYSTEM_H

#include "vsc_scriptsystem.h"
#include "veinqml.h"
#include <zeradblogger.h>
#include <testveinserver.h>
#include <QObject>
#include <memory>

static int constexpr systemEntityId = 0;
static int constexpr dataLoggerEntityId = 2;

class TestLoggerSystem
{
public:
    TestLoggerSystem();
    void setupServer();
    void cleanup();
    void setComponent(int entityId, QString componentName, QVariant newValue);
    static void waitForDbThread();
    QByteArray dumpStorage(QList<int> entities = QList<int>() << dataLoggerEntityId);
private:
    std::unique_ptr<TestVeinServer> m_server;
    VeinEvent::StorageSystem* m_storage;
    std::unique_ptr<VeinScript::ScriptSystem> m_scriptSystem;
    std::unique_ptr<VeinApiQml::VeinQml> m_qmlSystem;
    std::unique_ptr<ZeraDBLogger> m_dataLoggerSystem;
};

#endif // TESTLOGGERSYSTEM_H
