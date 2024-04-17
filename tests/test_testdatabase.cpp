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
    m_testSystem.waitForDbThread();

    QFile file(":/dumpDbOpenErrorEarly.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QByteArray jsonDumped = m_testSystem.dumpStorage();

    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::openDatabaseErrorLate()
{
    m_testSystem.setComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerDB::DBNameOpenErrorLate);
    m_testSystem.waitForDbThread();

    QFile file(":/dumpDbOpenErrorEarly.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QByteArray jsonDumped = m_testSystem.dumpStorage();

    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::openDatabaseOk()
{
    m_testSystem.setComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerDB::DBNameOpenOk);
    m_testSystem.waitForDbThread();

    QFile file(":/dumpDbOpenOk.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QByteArray jsonDumped = m_testSystem.dumpStorage();

    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::createSessionNoCustomerDataSystem()
{
    m_testSystem.setComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerDB::DBNameOpenOk);
    m_testSystem.waitForDbThread();

    QSignalSpy spy(TestLoggerDB::getInstance(), &TestLoggerDB::sigSessionStaticDataAdded);
    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "NotExistingDbSession");
    m_testSystem.waitForDbThread();

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
    m_testSystem.waitForDbThread();

    QSignalSpy spy(TestLoggerDB::getInstance(), &TestLoggerDB::sigSessionStaticDataAdded);
    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");
    m_testSystem.waitForDbThread();

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
    m_testSystem.waitForDbThread();

    QSignalSpy spySessionStaticComponents(TestLoggerDB::getInstance(), &TestLoggerDB::sigSessionStaticDataAdded);
    // DatabaseLogger::handleVeinDbSessionNameSet takes care on inserting entities/components in db
    QSignalSpy spyDbEntitiesAdded(TestLoggerDB::getInstance(), &TestLoggerDB::sigEntityAdded);
    QSignalSpy spyDbComponentsAdded(TestLoggerDB::getInstance(), &TestLoggerDB::sigComponentAdded);

    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "NotExistingDbSession");
    m_testSystem.waitForDbThread();

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
    m_testSystem.waitForDbThread();
    TestLoggerDB::getInstance()->setCustomerDataAlreadyInDbSession(true);

    QSignalSpy spySessionStaticComponents(TestLoggerDB::getInstance(), &TestLoggerDB::sigSessionStaticDataAdded);
    QSignalSpy spyDbEntitiesAdded(TestLoggerDB::getInstance(), &TestLoggerDB::sigEntityAdded);
    QSignalSpy spyDbComponentsAdded(TestLoggerDB::getInstance(), &TestLoggerDB::sigComponentAdded);

    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "NotExistingDbSession");
    m_testSystem.waitForDbThread();

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
    m_testSystem.waitForDbThread();

    QFile fileOpenOk(":/dumpDbOpenOk.json");
    QVERIFY(fileOpenOk.open(QFile::ReadOnly));
    QByteArray jsonExpected = fileOpenOk.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));

    TestLoggerDB::getInstance()->deleteDbFile();
    m_testSystem.waitForDbThread();

    QFile fileRemoved(":/dumpDbFileRemoved.json");
    QVERIFY(fileRemoved.open(QFile::ReadOnly));
    jsonExpected = fileRemoved.readAll();
    jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}
