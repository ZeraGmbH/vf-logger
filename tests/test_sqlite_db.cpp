#include "test_sqlite_db.h"
#include "testloggerdb.h"
#include <timemachineobject.h>
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
    loadDatabase();

    QSignalSpy spyDbEntitiesAdded(m_testSystem.getSignaller(), &TestDbAddSignaller::sigEntityAdded);
    QSignalSpy spyDbComponentsAdded(m_testSystem.getSignaller(), &TestDbAddSignaller::sigComponentAdded);

    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "NotExistingDbSession");

    QCOMPARE(spyDbEntitiesAdded.count(), 1);
    QCOMPARE(spyDbEntitiesAdded[0][0], 200);
    QCOMPARE(spyDbComponentsAdded.count(), 26);
}

void test_sqlite_db::createSessionWithCustomerDataAlreadyCreated()
{
    m_testSystem.setupServer();
    m_testSystem.appendCustomerDataSystem();
    loadDatabase();

    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "NotExistingDbSession1");
    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "NotExistingDbSession2");

    QSignalSpy spyDbEntitiesAdded(m_testSystem.getSignaller(), &TestDbAddSignaller::sigEntityAdded);
    QSignalSpy spyDbComponentsAdded(m_testSystem.getSignaller(), &TestDbAddSignaller::sigComponentAdded);

    QCOMPARE(spyDbEntitiesAdded.count(), 0);
    QCOMPARE(spyDbComponentsAdded.count(), 0);
}

void test_sqlite_db::loadDatabase()
{
    m_testSystem.setComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerDB::DBNameOpenOk);
}
