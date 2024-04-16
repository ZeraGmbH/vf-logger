#include "test_testdatabase.h"
#include <jsonloggercontentloader.h>
#include <jsonloggercontentsessionloader.h>
#include <loggercontentsetconfig.h>
#include "testdumpreporter.h"
#include <modulemanagersetupfacade.h>
#include "vl_datasource.h"
#include "vl_qmllogger.h"
#include "testloggerdb.h"
#include <timemachineobject.h>
#include <QBuffer>
#include <QThread>
#include <QSignalSpy>
#include <QTest>

QTEST_MAIN(test_testdatabase)

void test_testdatabase::initTestCase()
{
    VeinLogger::LoggerContentSetConfig::setJsonEnvironment(":/contentsets/", std::make_shared<JsonLoggerContentLoader>());
    VeinLogger::LoggerContentSetConfig::setJsonEnvironment(":/sessions/", std::make_shared<JsonLoggerContentSessionLoader>());
    ModuleManagerSetupFacade::registerMetaTypeStreamOperators();
}

void test_testdatabase::init()
{
    setupServer();
}

void test_testdatabase::cleanup()
{
    m_server = nullptr;
    m_dataLoggerSystem = nullptr;
    TimeMachineObject::feedEventLoop();
    m_scriptSystem = nullptr;
    TimeMachineObject::feedEventLoop();
}

static int constexpr systemEntityId = 0;
//static int constexpr scriptEntityId = 1;
static int constexpr dataLoggerEntityId = 2;

void test_testdatabase::openDatabaseErrorEarly()
{
    m_server->setComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerDB::DBNameOpenErrorEarly);
    waitForDbThread();

    QFile file(":/dumpDbOpenErrorEarly.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QByteArray jsonDumped;
    QBuffer buff(&jsonDumped);
    m_storage->dumpToFile(&buff, QList<int>() << dataLoggerEntityId);

    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::openDatabaseErrorLate()
{
    m_server->setComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerDB::DBNameOpenErrorLate);
    waitForDbThread();

    QFile file(":/dumpDbOpenErrorEarly.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QByteArray jsonDumped;
    QBuffer buff(&jsonDumped);
    m_storage->dumpToFile(&buff, QList<int>() << dataLoggerEntityId);

    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::openDatabaseOk()
{
    m_server->setComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerDB::DBNameOpenOk);
    waitForDbThread();

    QFile file(":/dumpDbOpenOk.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QByteArray jsonDumped;
    QBuffer buff(&jsonDumped);
    m_storage->dumpToFile(&buff, QList<int>() << dataLoggerEntityId);

    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::setSessionNotExistentInDb()
{
    m_server->setComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerDB::DBNameOpenOk);
    waitForDbThread();

    QSignalSpy spy(TestLoggerDB::getInstance(), &TestLoggerDB::sigSessionAdded);
    m_server->setComponent(dataLoggerEntityId, "sessionName", "foo");
    waitForDbThread();
    // no cutomerdata / statusmodule yet (see DatabaseLogger::handleVeinDbSessionNameSet for details)
    // => no dump on details on session static (created at start) data
    QCOMPARE(spy.count(), 1);

    QFile file(":/dumpDbSetSessionNew.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QByteArray jsonDumped;
    QBuffer buff(&jsonDumped);
    m_storage->dumpToFile(&buff, QList<int>() << dataLoggerEntityId);

    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::setupServer()
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

void test_testdatabase::waitForDbThread()
{
    // Ugly but we tried so much to receive signals from database thread e.g:
    // * QThread::yieldCurrentThread()
    // * Get TestLoggerDB object & QSignalSpy::Wait
    // did not work or were unreproducable
    TimeMachineObject::feedEventLoop();
    QThread::currentThread()->msleep(5);
    TimeMachineObject::feedEventLoop();
}
