#include "test_contentsets.h"
#include <jsonloggercontentloader.h>
#include <jsonloggercontentsessionloader.h>
#include <loggercontentsetconfig.h>
#include "testdumpreporter.h"
#include "vl_datasource.h"
#include "vl_qmllogger.h"
#include "vl_sqlitedb.h"
#include <timemachineobject.h>
#include <QBuffer>
#include <QTest>

QTEST_MAIN(test_contentsets)

void test_contentsets::initTestCase()
{
    VeinLogger::LoggerContentSetConfig::setJsonEnvironment(":/contentsets/", std::make_shared<JsonLoggerContentLoader>());
    VeinLogger::LoggerContentSetConfig::setJsonEnvironment(":/sessions/", std::make_shared<JsonLoggerContentSessionLoader>());
}

void test_contentsets::init()
{
    setupServer();
}

void test_contentsets::cleanup()
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

void test_contentsets::loggerSetupProperly()
{
    QFile file(":/dumpInitial.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QByteArray jsonDumped;
    QBuffer buff(&jsonDumped);
    m_storage->dumpToFile(&buff, QList<int>() << systemEntityId << dataLoggerEntityId << 10 << 11 << 12);

    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_contentsets::contentSetsSelectValid()
{
    m_server->setComponent(dataLoggerEntityId, "currentContentSets", "TestSet1"); // this is a bug - stored value must be a list!!!

    QFile file(":/dumpSetContentSetValidByString.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QByteArray jsonDumped;
    QBuffer buff(&jsonDumped);
    m_storage->dumpToFile(&buff, QList<int>() << dataLoggerEntityId);

    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_contentsets::contentSetsSelectInvalid()
{
    m_server->setComponent(dataLoggerEntityId, "currentContentSets", "foo");

    QFile file(":/dumpSetContentSetInvalid.json"); // another bug: we expect dumpInitial.json
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QByteArray jsonDumped;
    QBuffer buff(&jsonDumped);
    m_storage->dumpToFile(&buff, QList<int>() << dataLoggerEntityId);

    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_contentsets::contentSetsSelectValidList()
{
    m_server->setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "TestSet1");

    QFile file(":/dumpSetContentValid.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QByteArray jsonDumped;
    QBuffer buff(&jsonDumped);
    m_storage->dumpToFile(&buff, QList<int>() << dataLoggerEntityId);

    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_contentsets::contentSetsSelectValidListTwo()
{
    m_server->setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "TestSet1" << "TestSet2");

    QFile file(":/dumpSetContentValidTwo.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QByteArray jsonDumped;
    QBuffer buff(&jsonDumped);
    m_storage->dumpToFile(&buff, QList<int>() << dataLoggerEntityId);

    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_contentsets::contentSetsSelectValidListTwoSame()
{
    m_server->setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "TestSet1" << "TestSet1");

    QFile file(":/dumpSetContentValidTwoSame.json"); // we would not expect two identical...
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QByteArray jsonDumped;
    QBuffer buff(&jsonDumped);
    m_storage->dumpToFile(&buff, QList<int>() << dataLoggerEntityId);

    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_contentsets::contentSetsSelectValidListAll()
{
    m_server->setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "ZeraAll");

    QFile file(":/dumpSetContentAll.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QByteArray jsonDumped;
    QBuffer buff(&jsonDumped);
    m_storage->dumpToFile(&buff, QList<int>() << dataLoggerEntityId);

    // Just to remember: All Entities complete independent of context set configured
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_contentsets::setupServer()
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
        return new VeinLogger::SQLiteDB();
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
