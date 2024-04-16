#include "testloggersystem.h"
#include "testloggerdb.h"
#include "vl_qmllogger.h"
#include "vl_datasource.h"
#include "jsonloggercontentloader.h"
#include "jsonloggercontentsessionloader.h"
#include "loggercontentsetconfig.h"
#include <modulemanagersetupfacade.h>
#include <timemachineobject.h>
#include <QBuffer>
#include <QThread>

TestLoggerSystem::TestLoggerSystem()
{
    VeinLogger::LoggerContentSetConfig::setJsonEnvironment(":/contentsets/", std::make_shared<JsonLoggerContentLoader>());
    VeinLogger::LoggerContentSetConfig::setJsonEnvironment(":/sessions/", std::make_shared<JsonLoggerContentSessionLoader>());
    ModuleManagerSetupFacade::registerMetaTypeStreamOperators();
}

void TestLoggerSystem::setupServer()
{
    m_server = std::make_unique<TestVeinServer>();
    m_storage = m_server->getStorage();

    // in production modulemagaer there is this lambda hopping
    // here we can sequentialize by TimeMachineObject::feedEventLoop()
    for(int entityId = 10; entityId<13; entityId++) {
        QString entityName = QString("EntityName%1").arg(entityId);
        m_server->addEntity(entityId, entityName);
        for(int component=1; component<=3; component++) {
            QString componentName = QString("ComponentName%1").arg(component);
            m_server->addComponent(entityId, componentName, component, false);
        }
    }

    m_scriptSystem = std::make_unique<VeinScript::ScriptSystem>();
    m_server->appendEventSystem(m_scriptSystem.get());

    m_qmlSystem = std::make_unique<VeinApiQml::VeinQml>();
    VeinApiQml::VeinQml::setStaticInstance(m_qmlSystem.get());
    m_server->appendEventSystem(m_qmlSystem.get());
    TimeMachineObject::feedEventLoop();
    m_server->simulAllModulesLoaded("test-session.json", QStringList() << "test-session.json");

    const VeinLogger::DBFactory sqliteFactory = [](){
        return new TestLoggerDB();
    };
    m_dataLoggerSystem = std::make_unique<ZeraDBLogger>(new VeinLogger::DataSource(m_storage), sqliteFactory); //takes ownership of DataSource
    VeinLogger::QmlLogger::setStaticLogger(m_dataLoggerSystem.get());

    m_server->appendEventSystem(m_dataLoggerSystem.get());

    // subscribe those entitities our magic logger QML script requires
    m_qmlSystem->entitySubscribeById(systemEntityId);
    m_qmlSystem->entitySubscribeById(dataLoggerEntityId);

    TimeMachineObject::feedEventLoop();

    VeinLogger::DatabaseLogger::loadScripts(m_scriptSystem.get());
    TimeMachineObject::feedEventLoop();
}

void TestLoggerSystem::cleanup()
{
    m_server = nullptr;
    m_dataLoggerSystem = nullptr;
    TimeMachineObject::feedEventLoop();
    m_scriptSystem = nullptr;
    TimeMachineObject::feedEventLoop();
}

void TestLoggerSystem::setComponent(int entityId, QString componentName, QVariant newValue)
{
    m_server->setComponent(entityId, componentName, newValue);
}

void TestLoggerSystem::waitForDbThread()
{
    // Ugly but we tried so much to receive signals from database thread e.g:
    // * QThread::yieldCurrentThread()
    // * Get TestLoggerDB object & QSignalSpy::Wait
    // did not work or were unreproducable
    TimeMachineObject::feedEventLoop();
    QThread::currentThread()->msleep(5);
    TimeMachineObject::feedEventLoop();
}

QByteArray TestLoggerSystem::dumpStorage(QList<int> entities)
{
    QByteArray jsonDumped;
    QBuffer buff(&jsonDumped);
    m_storage->dumpToFile(&buff, entities);
    return jsonDumped;
}
