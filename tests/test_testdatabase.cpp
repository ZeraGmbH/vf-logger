#include "test_testdatabase.h"
#include "testloghelpers.h"
#include "testloggerdb.h"
#include <timemachineobject.h>
#include <QSignalSpy>
#include <QTest>

QTEST_MAIN(test_testdatabase)

void test_testdatabase::cleanup()
{
    m_testSystem.cleanup();
}


void test_testdatabase::openDatabaseErrorLate()
{
    m_testSystem.setupServer();
    m_testSystem.setComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerSystem::DBNameOpenErrorLate);

    QFile file(":/vein-dumps/dumpDbOpenErrorLate.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::createSessionNoCustomerDataSystem()
{
    m_testSystem.setupServer();
    m_testSystem.loadDatabase();
    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "NotExistingDbSession");

    QFile fileRecording(":/recording-dumps/dumpCreateSessionNoCustomerDataSystem.json");
    QVERIFY(fileRecording.open(QFile::ReadOnly));
    QByteArray jsonExpected = fileRecording.readAll();
    QByteArray jsonDumped = TestLoggerDB::getCurrentInstance()->getJsonDumpedComponentStored();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));

    QFile fileVein(":/vein-dumps/dumpDbSetSessionNew.json");
    QVERIFY(fileVein.open(QFile::ReadOnly));
    jsonExpected = fileVein.readAll();
    jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::setupCustomerData()
{
    m_testSystem.setupServer();
    m_testSystem.appendCustomerDataSystem();

    QFile file(":/vein-dumps/dumpDbCustomerDataInitial.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QByteArray jsonDumped = m_testSystem.dumpStorage(QList<int>() << dataLoggerEntityId << customerDataEntityId);

    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::selectExistingSession()
{
    m_testSystem.setupServer();
    m_testSystem.appendCustomerDataSystem();
    m_testSystem.loadDatabase();
    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");

    // see DatabaseLogger::handleVeinDbSessionNameSet: If a session is already existent
    // => no session static components added
    QFile fileRecording(":/recording-dumps/dumpRecordNotStarted.json");
    QVERIFY(fileRecording.open(QFile::ReadOnly));
    QByteArray jsonExpected = fileRecording.readAll();
    QByteArray jsonDumped = TestLoggerDB::getCurrentInstance()->getJsonDumpedComponentStored();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));

    QFile file(":/vein-dumps/dumpDbSetSessionAvail.json");
    QVERIFY(file.open(QFile::ReadOnly));
    jsonExpected = file.readAll();
    jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::createSession()
{
    m_testSystem.setupServer();
    m_testSystem.appendCustomerDataSystem();
    m_testSystem.loadDatabase();

    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "NotExistingDbSession");

    QFile fileRecording(":/recording-dumps/dumpCreateSession.json");
    QVERIFY(fileRecording.open(QFile::ReadOnly));
    QByteArray jsonExpected = fileRecording.readAll();
    QByteArray jsonDumped = TestLoggerDB::getCurrentInstance()->getJsonDumpedComponentStored();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));

    QFile fileVein(":/vein-dumps/dumpDbSetSessionNew.json");
    QVERIFY(fileVein.open(QFile::ReadOnly));
    QByteArray veinJsonExpected = fileVein.readAll();
    QByteArray veinJsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(veinJsonExpected, veinJsonDumped));
}

void test_testdatabase::recordVeinDump()
{
    m_testSystem.setupServer();
    m_testSystem.loadDatabase();
    m_testSystem.setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "TestSet1");
    m_testSystem.startLogging();

    QFile file(":/vein-dumps/dumpDbRecordInitial.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::recordOneContentSet()
{
    m_testSystem.setupServer(3, 3);
    m_testSystem.loadDatabase();
    m_testSystem.setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "TestSet1");
    m_testSystem.setComponentValues(1);

    m_testSystem.startLogging();

    m_testSystem.setComponentValues(2);

    QFile fileVein(":/vein-dumps/dumpDbRecordInitial.json");
    QVERIFY(fileVein.open(QFile::ReadOnly));
    QByteArray jsonExpected = fileVein.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));

    QFile file(":/recording-dumps/dumpRecordOneContentSet.json");
    QVERIFY(file.open(QFile::ReadOnly));
    jsonExpected = file.readAll();
    jsonDumped = TestLoggerDB::getCurrentInstance()->getJsonDumpedComponentStored();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::recordTwoContentSets()
{
    m_testSystem.setupServer(3, 3);
    m_testSystem.loadDatabase();
    m_testSystem.setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "TestSet1" << "TestSet2");
    m_testSystem.setComponentValues(1);

    m_testSystem.startLogging();

    m_testSystem.setComponentValues(2);

    QFile fileVein(":/vein-dumps/dumpRecordTwoContentSets.json");
    QVERIFY(fileVein.open(QFile::ReadOnly));
    QByteArray jsonExpected = fileVein.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));

    QFile file(":/recording-dumps/dumpRecordTwoContentSets.json");
    QVERIFY(file.open(QFile::ReadOnly));
    jsonExpected = file.readAll();
    jsonDumped = TestLoggerDB::getCurrentInstance()->getJsonDumpedComponentStored();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::recordAllContentSets()
{
    m_testSystem.setupServer(3, 3);
    m_testSystem.loadDatabase();
    m_testSystem.setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "ZeraAll");
    m_testSystem.setComponentValues(1);

    m_testSystem.startLogging();

    m_testSystem.setComponentValues(2);

    QFile fileVein(":/vein-dumps/dumpRecordAllContentSets.json");
    QVERIFY(fileVein.open(QFile::ReadOnly));
    QByteArray jsonExpected = fileVein.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));

    QFile file(":/recording-dumps/dumpRecordAllContentSets.json");
    QVERIFY(file.open(QFile::ReadOnly));
    jsonExpected = file.readAll();
    jsonDumped = TestLoggerDB::getCurrentInstance()->getJsonDumpedComponentStored();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::recordStartStop()
{
    recordAllContentSets();

    m_testSystem.stopLogging();

    m_testSystem.setComponentValues(10);

    QFile fileVein(":/vein-dumps/dumpRecordAllContentSetsStartStop.json");
    QVERIFY(fileVein.open(QFile::ReadOnly));
    QByteArray jsonExpected = fileVein.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));

    QFile file(":/recording-dumps/dumpRecordAllContentSetsStartStop.json");
    QVERIFY(file.open(QFile::ReadOnly));
    jsonExpected = file.readAll();
    jsonDumped = TestLoggerDB::getCurrentInstance()->getJsonDumpedComponentStored();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::noRecordTransactionMissing()
{
    m_testSystem.setupServer(3, 3);
    m_testSystem.loadDatabase();
    m_testSystem.setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "TestSet1");
    m_testSystem.setComponentValues(1);

    m_testSystem.startLogging("DbTestSession1", "");

    m_testSystem.setComponentValues(2);

    QFile fileVein(":/vein-dumps/dumpNoRecordTransactionMissing.json"); // bug on LoggingEnabled / Logging data
    QVERIFY(fileVein.open(QFile::ReadOnly));
    QByteArray jsonExpected = fileVein.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));

    QFile file(":/recording-dumps/dumpRecordNotStarted.json");
    QVERIFY(file.open(QFile::ReadOnly));
    jsonExpected = file.readAll();
    jsonDumped = TestLoggerDB::getCurrentInstance()->getJsonDumpedComponentStored();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::noRecordSessionMissing()
{
    m_testSystem.setupServer(3, 3);
    m_testSystem.loadDatabase();
    m_testSystem.setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "TestSet1");
    m_testSystem.setComponentValues(1);

    m_testSystem.startLogging("", "TestTransaction");

    m_testSystem.setComponentValues(2);

    QFile fileVein(":/vein-dumps/dumpNoRecordSessionMissing.json"); // bug on LoggingEnabled / Logging data
    QVERIFY(fileVein.open(QFile::ReadOnly));
    QByteArray jsonExpected = fileVein.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));

    QFile file(":/recording-dumps/dumpRecordNotStarted.json");
    QVERIFY(file.open(QFile::ReadOnly));
    jsonExpected = file.readAll();
    jsonDumped = TestLoggerDB::getCurrentInstance()->getJsonDumpedComponentStored();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::noRecordDatbaseMissing()
{
    m_testSystem.setupServer(3, 3);
    m_testSystem.setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "TestSet1"); // LOL
    m_testSystem.setComponentValues(1);

    m_testSystem.startLogging();

    m_testSystem.setComponentValues(2);

    // database is not created -> no dumps

    QFile fileVein(":/vein-dumps/dumpNoRecordDatabaseMissing.json");
    QVERIFY(fileVein.open(QFile::ReadOnly));
    QByteArray jsonExpected = fileVein.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::removeDbFileForUsbStickGone()
{
    m_testSystem.setupServer();
    m_testSystem.loadDatabase();

    QFile fileOpenOk(":/vein-dumps/dumpDbOpenOk.json");
    QVERIFY(fileOpenOk.open(QFile::ReadOnly));
    QByteArray jsonExpected = fileOpenOk.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));

    TestLoggerDB::getCurrentInstance()->deleteDbFile();
    TimeMachineObject::feedEventLoop();;

    QFile fileRemoved(":/vein-dumps/dumpDbFileRemoved.json");
    QVERIFY(fileRemoved.open(QFile::ReadOnly));
    jsonExpected = fileRemoved.readAll();
    jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::openRunLogAndClose()
{
    recordOneContentSet();

    m_testSystem.setComponent(dataLoggerEntityId, "DatabaseFile", "");

    QFile fileDbClose(":/vein-dumps/dumpOpenRunLogAndClose.json");
    QVERIFY(fileDbClose.open(QFile::ReadOnly));
    QByteArray jsonExpected = fileDbClose.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::guiContextMakesItIntoDbAndVein()
{
    m_testSystem.setupServer();
    m_testSystem.loadDatabase();
    QSignalSpy spy(m_testSystem.getSignaller(), &TestDbAddSignaller::sigAddTransaction);
    m_testSystem.setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "TestSet1");
    m_testSystem.setComponent(dataLoggerEntityId, "guiContext", "TestGuiContext");

    m_testSystem.startLogging();

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy[0][0], "TestTransaction");
    QCOMPARE(spy[0][1], "DbTestSession1");
    QCOMPARE(spy[0][2], "TestSet1");
    QCOMPARE(spy[0][3], "TestGuiContext");

    QFile file(":/vein-dumps/dumpDbGuiContext.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::removeTimeInfoInTransactions(QJsonObject &sessionInfo)
{
    for(QString transaction: sessionInfo.keys()) {
        QJsonObject temp = sessionInfo.value(transaction).toObject();
        temp.insert("Time", QString());
        sessionInfo.insert(transaction, temp);
    }
}
