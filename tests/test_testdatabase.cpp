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

    QFile file(":/vein-dumps/dumpDbOpenErrorEarly.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::openDatabaseErrorLate()
{
    m_testSystem.setComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerDB::DBNameOpenErrorLate);

    QFile file(":/vein-dumps/dumpDbOpenErrorLate.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::openDatabaseOk()
{
    loadDatabase();

    QFile file(":/vein-dumps/dumpDbOpenOk.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::createSessionNoCustomerDataSystem()
{
    loadDatabase();
    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "NotExistingDbSession");

    QFile fileRecording(":/recording-dumps/dumpCreateSessionNoCustomerDataSystem.json");
    QVERIFY(fileRecording.open(QFile::ReadOnly));
    QByteArray jsonExpected = fileRecording.readAll();
    QByteArray jsonDumped = TestLoggerDB::getInstance()->getJsonDumpedComponentStored();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));

    QFile fileVein(":/vein-dumps/dumpDbSetSessionNew.json");
    QVERIFY(fileVein.open(QFile::ReadOnly));
    jsonExpected = fileVein.readAll();
    jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::setupCustomerData()
{
    m_testSystem.appendCustomerDataSystem();

    QFile file(":/vein-dumps/dumpDbCustomerDataInitial.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QByteArray jsonDumped = m_testSystem.dumpStorage(QList<int>() << dataLoggerEntityId << customerDataEntityId);

    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::selectExistingSession()
{
    m_testSystem.appendCustomerDataSystem();
    loadDatabase();
    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");

    // see DatabaseLogger::handleVeinDbSessionNameSet: If a session is already existent
    // => no session static components added
    QFile fileRecording(":/recording-dumps/dumpSelectExistingSession.json");
    QVERIFY(fileRecording.open(QFile::ReadOnly));
    QByteArray jsonExpected = fileRecording.readAll();
    QByteArray jsonDumped = TestLoggerDB::getInstance()->getJsonDumpedComponentStored();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));

    QFile file(":/vein-dumps/dumpDbSetSessionAvail.json");
    QVERIFY(file.open(QFile::ReadOnly));
    jsonExpected = file.readAll();
    jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::createSession()
{
    m_testSystem.appendCustomerDataSystem();
    loadDatabase();

    // DatabaseLogger::handleVeinDbSessionNameSet takes care on inserting entities/components in db
    QSignalSpy spyDbEntitiesAdded(TestLoggerDB::getInstance(), &TestLoggerDB::sigEntityAdded);
    QSignalSpy spyDbComponentsAdded(TestLoggerDB::getInstance(), &TestLoggerDB::sigComponentAdded);

    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "NotExistingDbSession");

    QCOMPARE(spyDbEntitiesAdded.count(), 1);
    QCOMPARE(spyDbEntitiesAdded[0][0], 200);
    QCOMPARE(spyDbComponentsAdded.count(), 26);

    QFile fileRecording(":/recording-dumps/dumpCreateSession.json");
    QVERIFY(fileRecording.open(QFile::ReadOnly));
    QByteArray jsonExpected = fileRecording.readAll();
    QByteArray jsonDumped = TestLoggerDB::getInstance()->getJsonDumpedComponentStored();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));

    QFile fileVein(":/vein-dumps/dumpDbSetSessionNew.json");
    QVERIFY(fileVein.open(QFile::ReadOnly));
    QByteArray veinJsonExpected = fileVein.readAll();
    QByteArray veinJsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(veinJsonExpected, veinJsonDumped));
}

void test_testdatabase::createSessionWithCustomerDataAlreadyCreated()
{
    m_testSystem.appendCustomerDataSystem();
    loadDatabase();
    TestLoggerDB::getInstance()->setCustomerDataAlreadyInDbSession(true);

    QSignalSpy spyDbEntitiesAdded(TestLoggerDB::getInstance(), &TestLoggerDB::sigEntityAdded);
    QSignalSpy spyDbComponentsAdded(TestLoggerDB::getInstance(), &TestLoggerDB::sigComponentAdded);

    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "NotExistingDbSession");

    QCOMPARE(spyDbEntitiesAdded.count(), 0);
    QCOMPARE(spyDbComponentsAdded.count(), 0);

    QFile fileRecording(":/recording-dumps/dumpCreateSession.json");
    QVERIFY(fileRecording.open(QFile::ReadOnly));
    QByteArray jsonExpected = fileRecording.readAll();
    QByteArray jsonDumped = TestLoggerDB::getInstance()->getJsonDumpedComponentStored();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));

    QFile fileVein(":/vein-dumps/dumpDbSetSessionNew.json");
    QVERIFY(fileVein.open(QFile::ReadOnly));
    QByteArray veinJsonExpected = fileVein.readAll();

    QByteArray veinJsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(veinJsonExpected, veinJsonDumped));
}

void test_testdatabase::removeDbFileForUsbStickGone()
{
    loadDatabase();

    QFile fileOpenOk(":/vein-dumps/dumpDbOpenOk.json");
    QVERIFY(fileOpenOk.open(QFile::ReadOnly));
    QByteArray jsonExpected = fileOpenOk.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));

    TestLoggerDB::getInstance()->deleteDbFile();
    TimeMachineObject::feedEventLoop();;

    QFile fileRemoved(":/vein-dumps/dumpDbFileRemoved.json");
    QVERIFY(fileRemoved.open(QFile::ReadOnly));
    jsonExpected = fileRemoved.readAll();
    jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::openSelectExistingSessionAndClose()
{
    m_testSystem.appendCustomerDataSystem();
    loadDatabase();

    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");

    QFile fileVein(":/vein-dumps/dumpDbSetSessionAvail.json");
    QVERIFY(fileVein.open(QFile::ReadOnly));
    QByteArray jsonExpected = fileVein.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));

    m_testSystem.setComponent(dataLoggerEntityId, "DatabaseFile", "");
    QFile fileDbClose(":/vein-dumps/dumpDbOpenSetSessionAndClose.json");
    QVERIFY(fileDbClose.open(QFile::ReadOnly));
    jsonExpected = fileDbClose.readAll();
    jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::recordVeinDump()
{
    loadDatabase();
    m_testSystem.setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "TestSet1");
    startLogging();

    QFile file(":/vein-dumps/dumpDbRecordInitial.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::recordOneContentSet()
{
    loadDatabase();
    m_testSystem.setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "TestSet1");
    setComponentValues(1);

    startLogging();
    TestLoggerDB::getInstance()->valuesFromNowOnAreRecorded();

    setComponentValues(2);

    QFile file(":/recording-dumps/dumpRecordOneContentSet.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = TestLoggerDB::getInstance()->getJsonDumpedComponentStored();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::loadDatabase()
{
    m_testSystem.setComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerDB::DBNameOpenOk);
}

void test_testdatabase::setComponentValues(int valuesEmittedPerComponent)
{
    QMap<int, QList<QString>> componentsCreated = m_testSystem.getComponentsCreated();
    QList<int> entityIds = componentsCreated.keys();
    for(int i=1; i<=valuesEmittedPerComponent; i++) {
        for(int entityId : qAsConst(entityIds)) {
            QList<QString> components = componentsCreated[entityId];
            for(const QString &componentName : components) {
                QString value = QString("Entity: %1 / Component: %2 / Value: %3").arg(entityId).arg(componentName).arg(i);
                m_testSystem.setComponent(entityId, componentName, value);
            }
        }
    }
}

void test_testdatabase::startLogging()
{
    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");
    m_testSystem.setComponent(dataLoggerEntityId, "transactionName", "TestTransaction");
    m_testSystem.setComponent(dataLoggerEntityId, "LoggingEnabled", true);
}

