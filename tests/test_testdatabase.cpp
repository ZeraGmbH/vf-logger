#include "test_testdatabase.h"
#include "loggerstatictexts.h"
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

void test_testdatabase::createSessionNoCustomerDataSystem()
{
    m_testSystem.setupServer();
    m_testSystem.loadDatabase();
    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "NotExistingDbSession");

    QByteArray jsonDumped = TestLoggerDB::getCurrentInstance()->getJsonDumpedComponentStored();
    bool allOk = true;
    if (!TestLogHelpers::compareAndLogOnDiffJsonSrc("recording-dumps/dumpCreateSessionNoCustomerDataSystem.json", jsonDumped, TEST_SRC_DIR))
        allOk = false;

    jsonDumped = m_testSystem.dumpStorage();
    if (!TestLogHelpers::compareAndLogOnDiffJsonSrc("vein-dumps/dumpDbSetSessionNew.json", jsonDumped, TEST_SRC_DIR))
        allOk = false;
    QVERIFY(allOk);
}

void test_testdatabase::setupCustomerData()
{
    m_testSystem.setupServer();
    m_testSystem.appendCustomerDataSystem();

    QByteArray jsonDumped = m_testSystem.dumpStorage(QList<int>() << dataLoggerEntityId << customerDataEntityId);
    QVERIFY(TestLogHelpers::compareAndLogOnDiffJsonSrc("vein-dumps/dumpDbCustomerDataInitial.json", jsonDumped, TEST_SRC_DIR));
}

void test_testdatabase::selectExistingSession()
{
    m_testSystem.setupServer();
    m_testSystem.appendCustomerDataSystem();
    m_testSystem.loadDatabase();
    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");

    bool allOk = true;
    // see DatabaseLogger::handleVeinDbSessionNameSet: If a session is already existent
    // => no session static components added
    QByteArray jsonDumped = TestLoggerDB::getCurrentInstance()->getJsonDumpedComponentStored();
    if (!TestLogHelpers::compareAndLogOnDiffJsonSrc("recording-dumps/dumpRecordNotStarted.json", jsonDumped, TEST_SRC_DIR))
        allOk = false;

    jsonDumped = m_testSystem.dumpStorage();
    if (!TestLogHelpers::compareAndLogOnDiffJsonSrc("vein-dumps/dumpDbSetSessionAvail.json", jsonDumped, TEST_SRC_DIR))
        allOk = false;
    QVERIFY(allOk);
}

void test_testdatabase::createSession()
{
    m_testSystem.setupServer();
    m_testSystem.appendCustomerDataSystem();
    m_testSystem.loadDatabase();

    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "NotExistingDbSession");

    bool allOk = true;
    QByteArray jsonDumped = TestLoggerDB::getCurrentInstance()->getJsonDumpedComponentStored();
    if (!TestLogHelpers::compareAndLogOnDiffJsonSrc("recording-dumps/dumpCreateSession.json", jsonDumped, TEST_SRC_DIR))
        allOk = false;

    QByteArray veinJsonDumped = m_testSystem.dumpStorage();
    if (!TestLogHelpers::compareAndLogOnDiffJsonSrc("vein-dumps/dumpDbSetSessionNew.json", veinJsonDumped, TEST_SRC_DIR))
        allOk = false;
    QVERIFY(allOk);
}

void test_testdatabase::recordVeinDump()
{
    m_testSystem.setupServer();
    m_testSystem.loadDatabase();
    m_testSystem.setComponent(dataLoggerEntityId, LoggerStaticTexts::s_currentContentSetsComponentName, QVariantList() << "TestSet1");
    m_testSystem.startLogging();

    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiffJsonSrc("vein-dumps/dumpDbRecordInitial.json", jsonDumped, TEST_SRC_DIR));
}

void test_testdatabase::recordOneContentSet()
{
    m_testSystem.setupServer(3, 3);
    m_testSystem.loadDatabase();
    m_testSystem.setComponent(dataLoggerEntityId, LoggerStaticTexts::s_currentContentSetsComponentName, QVariantList() << "TestSet1");
    m_testSystem.setComponentValues(1);

    m_testSystem.startLogging();

    m_testSystem.setComponentValues(2);

    bool allOk = true;
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    if (!TestLogHelpers::compareAndLogOnDiffJsonSrc("vein-dumps/dumpDbRecordInitial.json", jsonDumped, TEST_SRC_DIR))
        allOk = false;

    jsonDumped = TestLoggerDB::getCurrentInstance()->getJsonDumpedComponentStored();
    if (!TestLogHelpers::compareAndLogOnDiffJsonSrc("recording-dumps/dumpRecordOneContentSet.json", jsonDumped, TEST_SRC_DIR))
        allOk = false;
    QVERIFY(allOk);
}

void test_testdatabase::recordTwoContentSets()
{
    m_testSystem.setupServer(3, 3);
    m_testSystem.loadDatabase();
    m_testSystem.setComponent(dataLoggerEntityId, LoggerStaticTexts::s_currentContentSetsComponentName, QVariantList() << "TestSet1" << "TestSet2");
    m_testSystem.setComponentValues(1);

    m_testSystem.startLogging();

    m_testSystem.setComponentValues(2);

    bool allOk = true;
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    if (!TestLogHelpers::compareAndLogOnDiffJsonSrc("vein-dumps/dumpRecordTwoContentSets.json", jsonDumped, TEST_SRC_DIR))
        allOk = false;

    jsonDumped = TestLoggerDB::getCurrentInstance()->getJsonDumpedComponentStored();
    if (!TestLogHelpers::compareAndLogOnDiffJsonSrc("recording-dumps/dumpRecordTwoContentSets.json", jsonDumped, TEST_SRC_DIR))
        allOk = false;
    QVERIFY(allOk);
}

void test_testdatabase::recordAllContentSets()
{
    m_testSystem.setupServer(3, 3);
    m_testSystem.loadDatabase();
    m_testSystem.setComponent(dataLoggerEntityId, LoggerStaticTexts::s_currentContentSetsComponentName, QVariantList() << "ZeraAll");
    m_testSystem.setComponentValues(1);

    m_testSystem.startLogging();

    m_testSystem.setComponentValues(2);

    bool allOk = true;
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    if (!TestLogHelpers::compareAndLogOnDiffJsonSrc("vein-dumps/dumpRecordAllContentSets.json", jsonDumped, TEST_SRC_DIR))
        allOk = false;

    jsonDumped = TestLoggerDB::getCurrentInstance()->getJsonDumpedComponentStored();
    if (!TestLogHelpers::compareAndLogOnDiffJsonSrc("recording-dumps/dumpRecordAllContentSets.json", jsonDumped, TEST_SRC_DIR))
        allOk = false;
    QVERIFY(allOk);
}

void test_testdatabase::recordStartStop()
{
    recordAllContentSets();

    m_testSystem.stopLogging();

    m_testSystem.setComponentValues(10);

    bool allOk = true;
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    if (!TestLogHelpers::compareAndLogOnDiffJsonSrc("vein-dumps/dumpRecordAllContentSetsStartStop.json", jsonDumped, TEST_SRC_DIR))
        allOk = false;

    jsonDumped = TestLoggerDB::getCurrentInstance()->getJsonDumpedComponentStored();
    if (!TestLogHelpers::compareAndLogOnDiffJsonSrc("recording-dumps/dumpRecordAllContentSetsStartStop.json", jsonDumped, TEST_SRC_DIR))
        allOk = false;
    QVERIFY(allOk);
}

void test_testdatabase::noRecordTransactionMissing()
{
    m_testSystem.setupServer(3, 3);
    m_testSystem.loadDatabase();
    m_testSystem.setComponent(dataLoggerEntityId, LoggerStaticTexts::s_currentContentSetsComponentName, QVariantList() << "TestSet1");
    m_testSystem.setComponentValues(1);

    m_testSystem.startLogging("DbTestSession1", "");

    m_testSystem.setComponentValues(2);

    bool allOk = true;
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    if (!TestLogHelpers::compareAndLogOnDiffJsonSrc("vein-dumps/dumpNoRecordTransactionMissing.json", jsonDumped, TEST_SRC_DIR)) // bug on LoggingEnabled / Logging data
        allOk = false;

    jsonDumped = TestLoggerDB::getCurrentInstance()->getJsonDumpedComponentStored();
    if (!TestLogHelpers::compareAndLogOnDiffJsonSrc("recording-dumps/dumpRecordNotStarted.json", jsonDumped, TEST_SRC_DIR))
        allOk = false;
    QVERIFY(allOk);
}

void test_testdatabase::noRecordSessionMissing()
{
    m_testSystem.setupServer(3, 3);
    m_testSystem.loadDatabase();
    m_testSystem.setComponent(dataLoggerEntityId, LoggerStaticTexts::s_currentContentSetsComponentName, QVariantList() << "TestSet1");
    m_testSystem.setComponentValues(1);

    m_testSystem.startLogging("", "TestTransaction");

    m_testSystem.setComponentValues(2);

    bool allOk = true;
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    if (!TestLogHelpers::compareAndLogOnDiffJsonSrc("vein-dumps/dumpNoRecordSessionMissing.json", jsonDumped, TEST_SRC_DIR)) // bug on LoggingEnabled / Logging data
        allOk = false;

    jsonDumped = TestLoggerDB::getCurrentInstance()->getJsonDumpedComponentStored();
    if (!TestLogHelpers::compareAndLogOnDiffJsonSrc("recording-dumps/dumpRecordNotStarted.json", jsonDumped, TEST_SRC_DIR))
        allOk = false;
    QVERIFY(allOk);
}

void test_testdatabase::noRecordDatbaseMissing()
{
    m_testSystem.setupServer(3, 3);
    m_testSystem.setComponent(dataLoggerEntityId, LoggerStaticTexts::s_currentContentSetsComponentName, QVariantList() << "TestSet1"); // LOL
    m_testSystem.setComponentValues(1);

    m_testSystem.startLogging();

    m_testSystem.setComponentValues(2);

    // database is not created -> no dumps

    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiffJsonSrc("vein-dumps/dumpNoRecordDatabaseMissing.json", jsonDumped, TEST_SRC_DIR));
}

void test_testdatabase::removeDbFileForUsbStickGone()
{
    m_testSystem.setupServer();
    m_testSystem.loadDatabase();

    bool allOk = true;
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    if (!TestLogHelpers::compareAndLogOnDiffJsonSrc("vein-dumps/dumpDbOpenOk.json", jsonDumped, TEST_SRC_DIR))
        allOk = false;

    TestLoggerDB::getCurrentInstance()->deleteDbFile();
    TimeMachineObject::feedEventLoop();;

    jsonDumped = m_testSystem.dumpStorage();
    if (!TestLogHelpers::compareAndLogOnDiffJsonSrc("vein-dumps/dumpDbFileRemoved.json", jsonDumped, TEST_SRC_DIR))
        allOk = false;
    QVERIFY(allOk);
}

void test_testdatabase::openRunLogAndClose()
{
    recordOneContentSet();

    m_testSystem.setComponent(dataLoggerEntityId, "DatabaseFile", "");

    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiffJsonSrc("vein-dumps/dumpOpenRunLogAndClose.json", jsonDumped, TEST_SRC_DIR));
}

void test_testdatabase::guiContextMakesItIntoDbAndVein()
{
    m_testSystem.setupServer();
    m_testSystem.loadDatabase();
    QSignalSpy spy(m_testSystem.getSignaller(), &TestDbAddSignaller::sigAddTransaction);
    m_testSystem.setComponent(dataLoggerEntityId, LoggerStaticTexts::s_currentContentSetsComponentName, QVariantList() << "TestSet1");
    m_testSystem.setComponent(dataLoggerEntityId, "guiContext", "TestGuiContext");

    m_testSystem.startLogging();

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy[0][0], "TestTransaction");
    QCOMPARE(spy[0][1], "DbTestSession1");
    QCOMPARE(spy[0][2], QStringList() << "TestSet1");
    QCOMPARE(spy[0][3], "TestGuiContext");

    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiffJsonSrc("vein-dumps/dumpDbGuiContext.json", jsonDumped, TEST_SRC_DIR));
}

void test_testdatabase::noRecordSessionChange()
{
    m_testSystem.setupServer();
    m_testSystem.loadDatabase();
    m_testSystem.setComponent(dataLoggerEntityId, LoggerStaticTexts::s_currentContentSetsComponentName, QVariantList() << "TestSet1");
    bool isLoggingEnabled = m_testSystem.getValueOfComponent(dataLoggerEntityId, "LoggingEnabled").toBool();
    QCOMPARE(isLoggingEnabled, false);

    m_testSystem.changeSession();
    isLoggingEnabled = m_testSystem.getValueOfComponent(dataLoggerEntityId, "LoggingEnabled").toBool();
    QCOMPARE(isLoggingEnabled, false);
}

void test_testdatabase::stopRecordingOnSessionChange()
{
    m_testSystem.setupServer();
    m_testSystem.loadDatabase();
    m_testSystem.setComponent(dataLoggerEntityId, LoggerStaticTexts::s_currentContentSetsComponentName, QVariantList() << "TestSet1");
    m_testSystem.startLogging();
    bool isLoggingEnabled = m_testSystem.getValueOfComponent(dataLoggerEntityId, "LoggingEnabled").toBool();
    QCOMPARE(isLoggingEnabled, true);

    m_testSystem.changeSession();
    isLoggingEnabled = m_testSystem.getValueOfComponent(dataLoggerEntityId, "LoggingEnabled").toBool();
    QCOMPARE(isLoggingEnabled, false);

}
