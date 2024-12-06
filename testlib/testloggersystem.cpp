#include "testloggersystem.h"
#include "testloggerdb.h"
#include "testsqlitedb.h"
#include "jsonloggercontentloader.h"
#include "jsonloggercontentsessionloader.h"
#include "loggercontentsetconfig.h"
#include <vs_dumpjson.h>
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

void TestLoggerSystem::setupServer(int entityCount, int componentCount, QList<int> entitiesWithAllComponentsStoredAlways)
{
    QDir dir;
    dir.mkpath(getCustomerDataPath());

    m_server = std::make_unique<TestVeinServer>();

    m_server->addTestEntities(entityCount, componentCount);
    m_testSignaller = std::make_unique<TestDbAddSignaller>();

    const VeinLogger::DBFactory sqliteFactory = [&]() -> VeinLogger::AbstractLoggerDB* {
        switch(m_dbType) {
        case MOCK:
            return new TestLoggerDB(m_testSignaller.get());
        case SQLITE:
            return new TestSQLiteDB(m_testSignaller.get());
        }
        return nullptr;
    };
    m_dataLoggerSystem = std::make_unique<VeinLogger::DatabaseLogger>(
        m_server->getStorage(),
        sqliteFactory,
        nullptr,
        entitiesWithAllComponentsStoredAlways);
    m_server->appendEventSystem(m_dataLoggerSystem.get());
    TimeMachineObject::feedEventLoop();

    m_server->simulAllModulesLoaded("test-session1.json", QStringList() << "test-session1.json" << "test-session2.json");
}

QMap<int, QList<QString> > TestLoggerSystem::getComponentsCreated()
{
    return m_server->getTestEntityComponentInfo();
}

void TestLoggerSystem::appendCustomerDataSystem()
{
    m_customerDataSystem = std::make_unique<CustomerDataSystem>(getCustomerDataPath());
    m_server->appendEventSystem(m_customerDataSystem.get());
    m_customerDataSystem->initializeEntity();
    TimeMachineObject::feedEventLoop();
}

void TestLoggerSystem::cleanup()
{
    TimeMachineObject::feedEventLoop();
    if(m_dataLoggerSystem) {
        m_dataLoggerSystem->closeDatabase();
        TimeMachineObject::feedEventLoop(); // deleteLater for DB and vein component updates
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

void TestLoggerSystem::setDataComponent(int entityId, QString componentName, QVariant newValue)
{
    // Server: We are not interested in changes only!!!
    m_server->setComponentServerNotification(entityId, componentName, newValue);
}

void TestLoggerSystem::setControlComponent(int entityId, QString componentName, QVariant newValue)
{
    // Client: Changes from server seem to be ignored - omg...
    m_server->setComponentClientTransaction(entityId, componentName, newValue);
}

void TestLoggerSystem::setComponentValuesSequenceEach(int valuesEmittedPerComponent)
{
    QMap<int, QList<QString>> componentsCreated = getComponentsCreated();
    QList<int> entityIds = componentsCreated.keys();
    for(int i=1; i<=valuesEmittedPerComponent; i++) {
        for(int entityId : qAsConst(entityIds)) {
            QList<QString> components = componentsCreated[entityId];
            for(const QString &componentName : components) {
                QString value = QString("Entity: %1 / Component: %2 / Value: %3").arg(entityId).arg(componentName).arg(i);
                setDataComponent(entityId, componentName, value);
            }
        }
    }
}

void TestLoggerSystem::loadDatabase()
{
    setControlComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerDB::DBNameOpenOk);
}

void TestLoggerSystem::startLogging(QString sessionName, QString transactionName)
{
    if(TestLoggerDB::getCurrentInstance())
        TestLoggerDB::getCurrentInstance()->valuesFromNowOnAreInitial();
    setControlComponent(dataLoggerEntityId, "sessionName", sessionName);
    setControlComponent(dataLoggerEntityId, "transactionName", transactionName);
    setControlComponent(dataLoggerEntityId, "LoggingEnabled", true);
    if(TestLoggerDB::getCurrentInstance())
        TestLoggerDB::getCurrentInstance()->valuesFromNowOnAreRecorded();
}

void TestLoggerSystem::stopLogging()
{
    setControlComponent(dataLoggerEntityId, "LoggingEnabled", false);
}

void TestLoggerSystem::setNextValueWriteCount(int newValueWriteCount)
{
    TestLoggerDB::getCurrentInstance()->setNextValueWriteCount(newValueWriteCount);
}

void TestLoggerSystem::changeSession(const QString &sessionPath, int baseEntityId)
{
    m_server->changeSession(sessionPath, baseEntityId);
}

QString TestLoggerSystem::getCustomerDataPath()
{
    return "/tmp/test-vf-logger-customerdata/";
}

TestDbAddSignaller *TestLoggerSystem::getSignaller()
{
    return m_testSignaller.get();
}

QByteArray TestLoggerSystem::dumpStorage(QList<int> entities)
{
    QByteArray jsonDumped;
    QBuffer buff(&jsonDumped);
    VeinStorage::DumpJson::dumpToFile(m_server->getStorage()->getDb(), &buff, entities);
    return jsonDumped;
}
