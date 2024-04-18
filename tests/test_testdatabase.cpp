#include "test_testdatabase.h"
#include "testdumpreporter.h"
#include "testloggerdb.h"
#include <timemachineobject.h>
#include <QSignalSpy>
#include <QTest>

QTEST_MAIN(test_testdatabase)

void test_testdatabase::init()
{
    m_testSystem.setupServer();
}

void test_testdatabase::cleanup()
{
    m_testSystem.cleanup();
}

void test_testdatabase::openDatabaseErrorEarly()
{
    m_testSystem.setComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerDB::DBNameOpenErrorEarly);

    QFile file(":/dumpDbOpenErrorEarly.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QByteArray jsonDumped = m_testSystem.dumpStorage();

    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::openDatabaseErrorLate()
{
    m_testSystem.setComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerDB::DBNameOpenErrorLate);

    QFile file(":/dumpDbOpenErrorEarly.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QByteArray jsonDumped = m_testSystem.dumpStorage();

    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::openDatabaseOk()
{
    m_testSystem.setComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerDB::DBNameOpenOk);

    QFile file(":/dumpDbOpenOk.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QByteArray jsonDumped = m_testSystem.dumpStorage();

    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::createSessionNoCustomerDataSystem()
{
    m_testSystem.setComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerDB::DBNameOpenOk);

    QSignalSpy spy(TestLoggerDB::getInstance(), &TestLoggerDB::sigSessionStaticDataAdded);
    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "NotExistingDbSession");

    QCOMPARE(spy.count(), 1); // is empty...
    QJsonObject componentDataSessionStatic = spy[0][0].toJsonObject();
    QVERIFY(componentDataSessionStatic.contains("NotExistingDbSession"));
    QVERIFY(componentDataSessionStatic["NotExistingDbSession"].isArray());
    QCOMPARE(componentDataSessionStatic["NotExistingDbSession"].toArray().size(), 0);

    QFile file(":/dumpDbSetSessionNew.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QByteArray jsonDumped = m_testSystem.dumpStorage();

    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::setupCustomerData()
{
    m_testSystem.appendCustomerDataSystem();

    QFile file(":/dumpDbCustomerDataInitial.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QByteArray jsonDumped = m_testSystem.dumpStorage(QList<int>() << dataLoggerEntityId << customerDataEntityId);

    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::selectExistingSession()
{
    m_testSystem.appendCustomerDataSystem();
    m_testSystem.setComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerDB::DBNameOpenOk);

    QSignalSpy spy(TestLoggerDB::getInstance(), &TestLoggerDB::sigSessionStaticDataAdded);
    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");

    // see DatabaseLogger::handleVeinDbSessionNameSet: If a session is already existent
    // => no session static components added
    QCOMPARE(spy.count(), 0);

    QFile file(":/dumpDbSetSessionAvail.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QByteArray jsonDumped = m_testSystem.dumpStorage();

    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::createSession()
{
    m_testSystem.appendCustomerDataSystem();
    m_testSystem.setComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerDB::DBNameOpenOk);

    QSignalSpy spySessionStaticComponents(TestLoggerDB::getInstance(), &TestLoggerDB::sigSessionStaticDataAdded);
    // DatabaseLogger::handleVeinDbSessionNameSet takes care on inserting entities/components in db
    QSignalSpy spyDbEntitiesAdded(TestLoggerDB::getInstance(), &TestLoggerDB::sigEntityAdded);
    QSignalSpy spyDbComponentsAdded(TestLoggerDB::getInstance(), &TestLoggerDB::sigComponentAdded);

    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "NotExistingDbSession");

    QCOMPARE(spySessionStaticComponents.count(), 1);
    QCOMPARE(spyDbEntitiesAdded.count(), 1);
    QCOMPARE(spyDbEntitiesAdded[0][0], 200);
    QCOMPARE(spyDbComponentsAdded.count(), 26);

    QFile fileComponentData(":/dumpComponentsOnSessionNewWithCustomerdata.json");
    QVERIFY(fileComponentData.open(QFile::ReadOnly));
    QByteArray componentsExpected = fileComponentData.readAll();

    QJsonObject componentsReceived = spySessionStaticComponents[0][0].toJsonObject();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(componentsExpected, QJsonDocument(componentsReceived).toJson()));

    QFile fileVein(":/dumpDbSetSessionNew.json");
    QVERIFY(fileVein.open(QFile::ReadOnly));
    QByteArray veinJsonExpected = fileVein.readAll();

    QByteArray veinJsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(veinJsonExpected, veinJsonDumped));
}

void test_testdatabase::createSessionWithCustomerDataAlreadyCreated()
{
    m_testSystem.appendCustomerDataSystem();
    m_testSystem.setComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerDB::DBNameOpenOk);
    TimeMachineObject::feedEventLoop();;
    TestLoggerDB::getInstance()->setCustomerDataAlreadyInDbSession(true);

    QSignalSpy spySessionStaticComponents(TestLoggerDB::getInstance(), &TestLoggerDB::sigSessionStaticDataAdded);
    QSignalSpy spyDbEntitiesAdded(TestLoggerDB::getInstance(), &TestLoggerDB::sigEntityAdded);
    QSignalSpy spyDbComponentsAdded(TestLoggerDB::getInstance(), &TestLoggerDB::sigComponentAdded);

    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "NotExistingDbSession");

    QCOMPARE(spySessionStaticComponents.count(), 1);
    QCOMPARE(spyDbEntitiesAdded.count(), 0);
    QCOMPARE(spyDbComponentsAdded.count(), 0);

    QFile fileComponentData(":/dumpComponentsOnSessionNewWithCustomerdata.json");
    QVERIFY(fileComponentData.open(QFile::ReadOnly));
    QByteArray componentsExpected = fileComponentData.readAll();

    QJsonObject componentsReceived = spySessionStaticComponents[0][0].toJsonObject();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(componentsExpected, QJsonDocument(componentsReceived).toJson()));

    QFile fileVein(":/dumpDbSetSessionNew.json");
    QVERIFY(fileVein.open(QFile::ReadOnly));
    QByteArray veinJsonExpected = fileVein.readAll();

    QByteArray veinJsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(veinJsonExpected, veinJsonDumped));
}

void test_testdatabase::removeDbFileForUsbStickGone()
{
    m_testSystem.setComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerDB::DBNameOpenOk);

    QFile fileOpenOk(":/dumpDbOpenOk.json");
    QVERIFY(fileOpenOk.open(QFile::ReadOnly));
    QByteArray jsonExpected = fileOpenOk.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));

    TestLoggerDB::getInstance()->deleteDbFile();
    TimeMachineObject::feedEventLoop();;

    QFile fileRemoved(":/dumpDbFileRemoved.json");
    QVERIFY(fileRemoved.open(QFile::ReadOnly));
    jsonExpected = fileRemoved.readAll();
    jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::openSelectExistingSessionAndClose()
{
    m_testSystem.appendCustomerDataSystem();
    m_testSystem.setComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerDB::DBNameOpenOk);

    QSignalSpy spy(TestLoggerDB::getInstance(), &TestLoggerDB::sigSessionStaticDataAdded);
    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");
    QCOMPARE(spy.count(), 0);

    QFile file(":/dumpDbSetSessionAvail.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));

    m_testSystem.setComponent(dataLoggerEntityId, "DatabaseFile", "");
    QFile fileDbClose(":/dumpDbOpenSetSessionAndClose.json");
    QVERIFY(fileDbClose.open(QFile::ReadOnly));
    jsonExpected = fileDbClose.readAll();
    jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::recordVeinDump()
{
    startLoggerWithComponents();

    QFile file(":/dumpDbRecordInitial.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::recordOneContentSet()
{
    startLoggerWithComponents();

    QFile file(":/dumpRecordOneContentSet.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = TestLoggerDB::getInstance()->getLoggedValues();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::startLoggerWithComponents()
{
    m_testSystem.setComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerDB::DBNameOpenOk);
    m_testSystem.setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "TestSet1");

    setInitialVeinComponents();

    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");
    m_testSystem.setComponent(dataLoggerEntityId, "transactionName", "TestTransaction");
    m_testSystem.setComponent(dataLoggerEntityId, "LoggingEnabled", true);

    setLoggerOnComponents();
}

void test_testdatabase::setInitialVeinComponents()
{
    for(int entityId=10; entityId<=12; entityId++) {
        m_testSystem.setComponent(entityId, "ComponentName1", 1);
        m_testSystem.setComponent(entityId, "ComponentName2", 2);
        m_testSystem.setComponent(entityId, "ComponentName3", 3);
    }
}

void test_testdatabase::setLoggerOnComponents()
{
    for(int i=1; i<=2; i++) {
        for(int entityId=10; entityId<=12; entityId++) {
            m_testSystem.setComponent(entityId, "ComponentName1", i+entityId+0);
            m_testSystem.setComponent(entityId, "ComponentName2", i+entityId+1);
            m_testSystem.setComponent(entityId, "ComponentName3", i+entityId+2);
        }
    }
}
