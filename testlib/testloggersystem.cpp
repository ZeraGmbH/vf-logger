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
#include <QDir>

TestLoggerSystem::TestLoggerSystem()
{
    VeinLogger::LoggerContentSetConfig::setJsonEnvironment(":/contentsets/", std::make_shared<JsonLoggerContentLoader>());
    VeinLogger::LoggerContentSetConfig::setJsonEnvironment(":/sessions/", std::make_shared<JsonLoggerContentSessionLoader>());
}

void TestLoggerSystem::setupServer(int entityCount, int componentCount)
{
    QDir dir;
    dir.mkpath(getCustomerDataPath());

    m_server = std::make_unique<TestVeinServer>();
    m_storage = m_server->getStorage();

    m_server->addTestEntities(entityCount, componentCount);

    // in production modulemanager there is this lambda hopping
    // here we can sequentialize by TimeMachineObject::feedEventLoop()
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
    m_dataLoggerSystem = std::make_unique<VeinLogger::DatabaseLogger>(new VeinLogger::DataSource(m_storage), sqliteFactory); //takes ownership of DataSource
    VeinLogger::QmlLogger::setStaticLogger(m_dataLoggerSystem.get());

    m_server->appendEventSystem(m_dataLoggerSystem.get());

    // subscribe those entitities our magic logger QML script requires
    m_qmlSystem->entitySubscribeById(systemEntityId);
    m_qmlSystem->entitySubscribeById(dataLoggerEntityId);

    TimeMachineObject::feedEventLoop();

    VeinLogger::DatabaseLogger::loadScripts(m_scriptSystem.get());
    TimeMachineObject::feedEventLoop();
}


void TestLoggerSystem::removeEntitiesAddedInSetup()
{
    m_server->removeEntitiesAdded();
}

void TestLoggerSystem::appendCustomerDataSystem()
{
    m_customerDataSystem = std::make_unique<CustomerDataSystem>(getCustomerDataPath());
    m_server->appendEventSystem(m_customerDataSystem.get());
    m_customerDataSystem->initializeEntity();
    TimeMachineObject::feedEventLoop();
}

QString TestLoggerSystem::getCustomerDataPath()
{
    return "/tmp/test-vf-logger-customerdata/";
}

void TestLoggerSystem::cleanup()
{
    m_server = nullptr;
    m_dataLoggerSystem = nullptr;
    TimeMachineObject::feedEventLoop();
    m_scriptSystem = nullptr;
    TimeMachineObject::feedEventLoop();
    m_customerDataSystem = nullptr;
    TimeMachineObject::feedEventLoop();
    QDir dir(getCustomerDataPath());
    dir.removeRecursively();
}

void TestLoggerSystem::setComponent(int entityId, QString componentName, QVariant newValue)
{
    m_server->setComponent(entityId, componentName, newValue);
}

QMap<int, QList<QString> > TestLoggerSystem::getComponentsCreated()
{
    return m_server->getTestEntityComponentInfo();
}

QByteArray TestLoggerSystem::dumpStorage(QList<int> entities)
{
    QByteArray jsonDumped;
    QBuffer buff(&jsonDumped);
    m_storage->dumpToFile(&buff, entities);
    return jsonDumped;
}
