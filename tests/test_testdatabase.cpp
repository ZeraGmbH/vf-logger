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

void test_testdatabase::setSessionNotExistentInDb()
{
    m_testSystem.setComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerDB::DBNameOpenOk);
    m_testSystem.waitForDbThread();

    QSignalSpy spy(TestLoggerDB::getInstance(), &TestLoggerDB::sigSessionAdded);
    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "foo");
    m_testSystem.waitForDbThread();
    // no cutomerdata / statusmodule yet (see DatabaseLogger::handleVeinDbSessionNameSet for details)
    // => no dump on details on session static (created at start) data
    QCOMPARE(spy.count(), 1);

    QFile file(":/dumpDbSetSessionNew.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QByteArray jsonDumped = m_testSystem.dumpStorage();

    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_testdatabase::setSessionExistentInDb()
{
    m_testSystem.setComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerDB::DBNameOpenOk);
    m_testSystem.waitForDbThread();

    QSignalSpy spy(TestLoggerDB::getInstance(), &TestLoggerDB::sigSessionAdded);
    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");
    m_testSystem.waitForDbThread();

    QCOMPARE(spy.count(), 0); // Already existing in db -> no session added

    QFile file(":/dumpDbSetSessionAvail.json");
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

void test_testdatabase::setSessionNotExistentWithCustomerData()
{
    m_testSystem.appendCustomerDataSystem();
    m_testSystem.setComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerDB::DBNameOpenOk);
    m_testSystem.waitForDbThread();

    QSignalSpy spy(TestLoggerDB::getInstance(), &TestLoggerDB::sigSessionAdded);
    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "foo");
    m_testSystem.waitForDbThread();
    // no cutomerdata / statusmodule yet (see DatabaseLogger::handleVeinDbSessionNameSet for details)
    // => no dump on details on session static (created at start) data
    QCOMPARE(spy.count(), 1);

    QFile file(":/dumpDbSetSessionNew.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QByteArray jsonDumped = m_testSystem.dumpStorage();

    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}
