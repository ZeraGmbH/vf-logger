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

    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "NotExistingDbSession");

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

    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "NotExistingDbSession1");
    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "NotExistingDbSession2");

    QSignalSpy spyDbEntitiesAdded(m_testSystem.getSignaller(), &TestDbAddSignaller::sigEntityAdded);
    QSignalSpy spyDbComponentsAdded(m_testSystem.getSignaller(), &TestDbAddSignaller::sigComponentAdded);

    QCOMPARE(spyDbEntitiesAdded.count(), 0);
    QCOMPARE(spyDbComponentsAdded.count(), 0);
}

void test_sqlite_db::logInsertsEntityComponents()
{
    m_testSystem.setupServer();
    m_testSystem.loadDatabase();
    m_testSystem.setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "ZeraAll");
    QSignalSpy spyDbEntitiesAdded(m_testSystem.getSignaller(), &TestDbAddSignaller::sigEntityAdded);
    QSignalSpy spyDbComponentsAdded(m_testSystem.getSignaller(), &TestDbAddSignaller::sigComponentAdded);

    m_testSystem.setComponentValues(1);
    QCOMPARE(spyDbEntitiesAdded.count(), 0);
    QCOMPARE(spyDbComponentsAdded.count(), 0);

    m_testSystem.startLogging();
    QFile file(":/sqlite-inserts/dumpRecordTwoEntityComponents.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = TestInsertSpiesToJson::spiesToJsonAndClear(spyDbEntitiesAdded, spyDbComponentsAdded);
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));

    m_testSystem.setComponentValues(2);

    QCOMPARE(spyDbEntitiesAdded.count(), 0);
    QCOMPARE(spyDbComponentsAdded.count(), 0);
}

void test_sqlite_db::displaySessionInfo()
{
    m_testSystem.setupServer();
    m_testSystem.loadDatabase();
    m_testSystem.setComponent(dataLoggerEntityId, "sessionName", "Session1");
    m_testSystem.setComponent(dataLoggerEntityId, "guiContext", QVariantList() << "ZeraGuiActualValues");
    m_testSystem.setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "ZeraAll");

    m_testSystem.startLogging();
    m_testSystem.stopLogging();
    QString transactionName = m_testSystem.getValueOfComponent(dataLoggerEntityId, "transactionName").toString();
    QStringList guiContexts = m_testSystem.getValueOfComponent(dataLoggerEntityId, "guiContext").toStringList();
    QString guiContext = guiContexts[0];
    QStringList contentSets = m_testSystem.getValueOfComponent(dataLoggerEntityId, "currentContentSets").toStringList();
    QString contentSet = contentSets[0];

    QFile file(":/sqlite-inserts/dumpSessionInfos.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QJsonObject jsonObj = m_testSystem.displaySessionsInfos("Session1", transactionName, guiContext, contentSet);
    QJsonDocument jsonDoc(jsonObj);
    QString jsonDumped = jsonDoc.toJson(QJsonDocument::Indented);
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}
