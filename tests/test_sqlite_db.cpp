#include "test_sqlite_db.h"
#include "testloghelpers.h"
#include "testinsertspiestojson.h"
#include <timemachineobject.h>
#include <QFile>
#include <QSignalSpy>
#include <QTest>

QTEST_MAIN(test_sqlite_db)


test_sqlite_db::test_sqlite_db() :
    m_testSystem(TestLoggerSystem::SQLITE)
{
}

void test_sqlite_db::cleanup()
{
    m_testSystem.cleanup();
}

void test_sqlite_db::createSessionInsertsEntityComponents()
{
    m_testSystem.setupServer();
    m_testSystem.appendCustomerDataSystem();
    m_testSystem.loadDatabase();

    QSignalSpy spyDbEntitiesAdded(m_testSystem.getSignaller(), &TestDbAddSignaller::sigEntityAdded);
    QSignalSpy spyDbComponentsAdded(m_testSystem.getSignaller(), &TestDbAddSignaller::sigComponentAdded);

    m_testSystem.setControlComponent(dataLoggerEntityId, "sessionName", "NotExistingDbSession");

    QFile file(":/sqlite-inserts/dumpSessionCreated.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = TestInsertSpiesToJson::spiesToJsonAndClear(spyDbEntitiesAdded, spyDbComponentsAdded);
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_sqlite_db::createSessionWithCustomerDataAlreadyCreated()
{
    m_testSystem.setupServer();
    m_testSystem.appendCustomerDataSystem();
    m_testSystem.loadDatabase();

    m_testSystem.setControlComponent(dataLoggerEntityId, "sessionName", "NotExistingDbSession1");
    m_testSystem.setControlComponent(dataLoggerEntityId, "sessionName", "NotExistingDbSession2");

    QSignalSpy spyDbEntitiesAdded(m_testSystem.getSignaller(), &TestDbAddSignaller::sigEntityAdded);
    QSignalSpy spyDbComponentsAdded(m_testSystem.getSignaller(), &TestDbAddSignaller::sigComponentAdded);

    QCOMPARE(spyDbEntitiesAdded.count(), 0);
    QCOMPARE(spyDbComponentsAdded.count(), 0);
}

void test_sqlite_db::logInsertsEntityComponents()
{
    m_testSystem.setupServer();
    m_testSystem.loadDatabase();
    m_testSystem.setControlComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "ZeraAll");
    QSignalSpy spyDbEntitiesAdded(m_testSystem.getSignaller(), &TestDbAddSignaller::sigEntityAdded);
    QSignalSpy spyDbComponentsAdded(m_testSystem.getSignaller(), &TestDbAddSignaller::sigComponentAdded);

    m_testSystem.setComponentValuesSequenceEach(1);
    QCOMPARE(spyDbEntitiesAdded.count(), 0);
    QCOMPARE(spyDbComponentsAdded.count(), 0);

    m_testSystem.startLogging();
    QFile file(":/sqlite-inserts/dumpRecordTwoEntityComponents.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = TestInsertSpiesToJson::spiesToJsonAndClear(spyDbEntitiesAdded, spyDbComponentsAdded);
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));

    m_testSystem.setComponentValuesSequenceEach(2);

    QCOMPARE(spyDbEntitiesAdded.count(), 0);
    QCOMPARE(spyDbComponentsAdded.count(), 0);
}
