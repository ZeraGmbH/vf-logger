#include "testloggersystem.h"
#include "testloggerdb.h"
#include "testsqlitedb.h"
#include "loggercontentsetconfig.h"
#include "contentsetsotherfromcontentsetsconfig.h"
#include "contentsetszeraallfrommodmansessions.h"
#include <vs_dumpjson.h>
#include <modulemanagersetupfacade.h>
#include <timemachineobject.h>
#include <QBuffer>
#include <QThread>
#include <QDir>


const QLatin1String TestLoggerSystem::DBNameOpenOk = QLatin1String("/tmp/veindb-test/DB_NAME_OPEN_OK");
const QLatin1String TestLoggerSystem::DBNameOpenErrorEarly = QLatin1String("DB_NAME_OPEN_ERR");
const QLatin1String TestLoggerSystem::DBNameOpenErrorLate = QLatin1String("/tmp/DB_NAME_OPEN_ERR");

TestLoggerSystem::TestLoggerSystem(DbType dbType) :
    m_dbType(dbType)
{
    VeinLogger::LoggerContentSetConfig::setJsonEnvironment(":/contentsets/", std::make_shared<ContentSetsOtherFromContentSetsConfig>());
    VeinLogger::LoggerContentSetConfig::setJsonEnvironment(":/sessions/", std::make_shared<ContentSetsZeraAllFromModmanSessions>());
}

void TestLoggerSystem::setupServer(int entityCount,
                                   int componentCount,
                                   QList<int> entitiesWithAllComponentsStoredAlways)
{
    QDir dir;
    dir.mkpath(getCustomerDataPath());

    m_server = std::make_unique<TestVeinServer>();

    m_server->addTestEntities(entityCount, componentCount);
    m_testSignaller = std::make_unique<TestDbAddSignaller>();

    const VeinLogger::DBFactory sqliteFactory = [=]() -> std::shared_ptr<VeinLogger::AbstractLoggerDB> {
        switch(m_dbType) {
        case MOCK:
            return std::make_shared<TestLoggerDB>(m_testSignaller.get());
        case SQLITE:
            return std::make_shared<TestSQLiteDB>(m_testSignaller.get());
        }
        return nullptr;
    };
    m_dataLoggerSystem = std::make_unique<VeinLogger::DatabaseLogger>(m_server->getStorage(),
                                                                      sqliteFactory,
                                                                      nullptr,
                                                                      entitiesWithAllComponentsStoredAlways);
    m_server->appendEventSystem(m_dataLoggerSystem.get());
    TimeMachineObject::feedEventLoop();

    m_server->simulAllModulesLoaded("test-session1.json", QStringList() << "test-session1.json" << "test-session2.json");
}

TestVeinServer *TestLoggerSystem::getServer()
{
    return m_server.get();
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

void TestLoggerSystem::appendEventSystem(VeinEvent::EventSystem *system)
{
    m_server->appendEventSystem(system);
}

void TestLoggerSystem::cleanup()
{
    m_emitCountTotal = 0;
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
    QFile::remove(DBNameOpenOk);
}

void TestLoggerSystem::setComponent(int entityId, QString componentName, QVariant newValue)
{
    m_server->setComponentClientTransaction(entityId, componentName, newValue);
}

void TestLoggerSystem::setComponentValues(int valuesEmittedPerComponent)
{
    QMap<int, QList<QString>> componentsCreated = getComponentsCreated();
    QList<int> entityIds = componentsCreated.keys();
    for(int i=1; i<=valuesEmittedPerComponent; i++) {
        for(int entityId : qAsConst(entityIds)) {
            QList<QString> components = componentsCreated[entityId];
            for(const QString &componentName : components) {
                QString value = QString("Entity: %1 / Component: %2 / Value: %3").
                                arg(entityId).arg(componentName).arg(m_emitCountTotal+i);
                setComponent(entityId, componentName, value);
            }
        }
    }
    m_emitCountTotal += valuesEmittedPerComponent;
}

QVariant TestLoggerSystem::getValueOfComponent(int entityId, QString componentName)
{
    return m_server->getValue(entityId, componentName);
}

void TestLoggerSystem::loadDatabase()
{
    setComponent(dataLoggerEntityId, "DatabaseFile", DBNameOpenOk);
    TimeMachineObject::feedEventLoop();
}

void TestLoggerSystem::startLogging(QString sessionName, QString transactionName)
{
    if(TestLoggerDB::getCurrentInstance())
        TestLoggerDB::getCurrentInstance()->valuesFromNowOnAreInitial();
    setComponent(dataLoggerEntityId, "sessionName", sessionName);
    setComponent(dataLoggerEntityId, "transactionName", transactionName);
    setComponent(dataLoggerEntityId, "LoggingEnabled", true);
    if(TestLoggerDB::getCurrentInstance())
        TestLoggerDB::getCurrentInstance()->valuesFromNowOnAreRecorded();
    TimeMachineObject::feedEventLoop();
}

void TestLoggerSystem::stopLogging()
{
    setComponent(dataLoggerEntityId, "LoggingEnabled", false);
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

VeinLogger::DatabaseLogger *TestLoggerSystem::getDbLogger()
{
    return m_dataLoggerSystem.get();
}

TestDbAddSignaller *TestLoggerSystem::getSignaller()
{
    return m_testSignaller.get();
}

VeinStorage::AbstractEventSystem *TestLoggerSystem::getStorage()
{
    return m_server->getStorage();
}

QByteArray TestLoggerSystem::dumpStorage(QList<int> entities)
{
    QByteArray jsonDumped;
    QBuffer buff(&jsonDumped);
    VeinStorage::DumpJson::dumpToFile(m_server->getStorage()->getDb(), &buff, entities);
    return jsonDumped;
}
