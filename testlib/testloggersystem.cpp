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

void TestLoggerSystem::setupServer(int entityCount, int componentCount)
{
    QDir dir;
    dir.mkpath(getCustomerDataPath());

    m_server = std::make_unique<TestVeinServer>();
    m_storage = m_server->getStorage();

    m_server->addTestEntities(entityCount, componentCount);

    const VeinLogger::DBFactory sqliteFactory = [&]() {
        m_testSignaller = std::make_unique<TestDbAddSignaller>();
        switch(m_dbType) {
        case MOCK:
            m_db = new TestLoggerDB(m_testSignaller.get());
            break;
        case SQLITE:
            m_db = new TestSQLiteDB(m_testSignaller.get());
            break;
        }
        return m_db;
    };
    m_dataLoggerSystem = std::make_unique<VeinLogger::DatabaseLogger>(m_storage, sqliteFactory);
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
    m_server = nullptr;
    m_dataLoggerSystem = nullptr;
    TimeMachineObject::feedEventLoop();
    m_customerDataSystem = nullptr;
    TimeMachineObject::feedEventLoop();
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
    m_storage->dumpToFile(&buff, entities);
    return jsonDumped;
}
