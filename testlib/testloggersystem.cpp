#include "testloggersystem.h"
#include "testloggerdb.h"
#include "testsqlitedb.h"
#include "jsonloggercontentloader.h"
#include "jsonloggercontentsessionloader.h"
#include "loggercontentsetconfig.h"
#include <modulemanagersetupfacade.h>
#include <timemachineobject.h>
#include <QBuffer>
#include <QThread>
#include <QDir>

TestLoggerSystem::TestLoggerSystem(DbType dbType) :
    m_dbType(dbType)
{
    VeinLogger::LoggerContentSetConfig::setJsonEnvironment(":/contentsets/", std::make_shared<JsonLoggerContentLoader>());
    VeinLogger::LoggerContentSetConfig::setJsonEnvironment(":/sessions/", std::make_shared<JsonLoggerContentSessionLoader>());
}

void TestLoggerSystem::loadDatabase()
{
    setComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerDB::DBNameOpenOk);
}

void TestLoggerSystem::setComponentValues(int valuesEmittedPerComponent)
{
    QMap<int, QList<QString>> componentsCreated = getComponentsCreated();
    QList<int> entityIds = componentsCreated.keys();
    for(int i=1; i<=valuesEmittedPerComponent; i++) {
        for(int entityId : qAsConst(entityIds)) {
            QList<QString> components = componentsCreated[entityId];
            for(const QString &componentName : components) {
                QString value = QString("Entity: %1 / Component: %2 / Value: %3").arg(entityId).arg(componentName).arg(i);
                setComponent(entityId, componentName, value);
            }
        }
    }
}

void TestLoggerSystem::startLogging(QString sessionName, QString transactionName)
{
    setComponent(dataLoggerEntityId, "sessionName", sessionName);
    setComponent(dataLoggerEntityId, "transactionName", transactionName);
    setComponent(dataLoggerEntityId, "LoggingEnabled", true);
}

void TestLoggerSystem::setupServer(int entityCount, int componentCount)
{
    QDir dir;
    dir.mkpath(getCustomerDataPath());

    m_server = std::make_unique<TestVeinServer>();

    m_server->addTestEntities(entityCount, componentCount);
    m_testSignaller = std::make_unique<TestDbAddSignaller>();

    const VeinLogger::DBFactory sqliteFactory = [=]() -> VeinLogger::AbstractLoggerDB* {
        switch(m_dbType) {
        case MOCK:
            return new TestLoggerDB(m_testSignaller.get());
        case SQLITE:
            return new TestSQLiteDB(m_testSignaller.get());
        }
        return nullptr;
    };
    m_dataLoggerSystem = std::make_unique<VeinLogger::DatabaseLogger>(m_server->getStorage(), sqliteFactory);
    m_server->appendEventSystem(m_dataLoggerSystem.get());
    TimeMachineObject::feedEventLoop();

    m_server->simulAllModulesLoaded("test-session1.json", QStringList() << "test-session1.json" << "test-session2.json");
}

void TestLoggerSystem::changeSession(const QString &sessionPath, int baseEntityId)
{
    m_server->changeSession(sessionPath, baseEntityId);
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
    TimeMachineObject::feedEventLoop();
    if(m_dataLoggerSystem) {
        m_server->getEventHandler()->removeSubsystem(m_dataLoggerSystem.get());
        m_dataLoggerSystem = nullptr;
    }
    if(m_customerDataSystem) {
        m_server->getEventHandler()->removeSubsystem(m_customerDataSystem.get());
        m_customerDataSystem = nullptr;
    }
    m_server = nullptr;
    m_testSignaller = nullptr;

    QDir dirCustomer(getCustomerDataPath());
    dirCustomer.removeRecursively();
    QFile::remove(TestLoggerDB::DBNameOpenOk);
}

TestDbAddSignaller *TestLoggerSystem::getSignaller()
{
    return m_testSignaller.get();
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
    m_server->getStorage()->dumpToFile(&buff, entities);
    return jsonDumped;
}
