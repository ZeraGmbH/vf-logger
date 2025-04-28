#include "test_mockandsqlitedatabase.h"
#include "testloghelpers.h"
#include "testinsertspiestojson.h"
#include <QTest>

Q_DECLARE_METATYPE(TestLoggerSystem::DbType)
QTEST_MAIN(test_mockandsqlitedatabase)

void test_mockandsqlitedatabase::initTestCase_data()
{
    QTest::addColumn<TestLoggerSystem::DbType>("DbType");
    QTest::newRow("Mock Databse") << TestLoggerSystem::DbType::MOCK;
    QTest::newRow("SQLite Databse") << TestLoggerSystem::DbType::SQLITE;
}

void test_mockandsqlitedatabase::init()
{
    QFETCH_GLOBAL(TestLoggerSystem::DbType, DbType);
    m_testSystem = std::make_unique<TestLoggerSystem>(DbType);
}

void test_mockandsqlitedatabase::cleanup()
{
    m_testSystem->cleanup();
    m_testSystem.reset();
}

void test_mockandsqlitedatabase::createSessionInsertsEntityComponents()
{
    m_testSystem->setupServer();
    m_testSystem->appendCustomerDataSystem();
    m_testSystem->loadDatabase();

    QSignalSpy spyDbEntitiesAdded(m_testSystem->getSignaller(), &TestDbAddSignaller::sigEntityAdded);
    QSignalSpy spyDbComponentsAdded(m_testSystem->getSignaller(), &TestDbAddSignaller::sigComponentAdded);

    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "NotExistingDbSession");

    QFile file(":/sqlite-inserts/dumpSessionCreated.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = TestInsertSpiesToJson::spiesToJsonAndClear(spyDbEntitiesAdded, spyDbComponentsAdded);
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_mockandsqlitedatabase::createSessionWithCustomerDataAlreadyCreated()
{
    m_testSystem->setupServer();
    m_testSystem->appendCustomerDataSystem();
    m_testSystem->loadDatabase();

    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "NotExistingDbSession1");
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "NotExistingDbSession2");

    QSignalSpy spyDbEntitiesAdded(m_testSystem->getSignaller(), &TestDbAddSignaller::sigEntityAdded);
    QSignalSpy spyDbComponentsAdded(m_testSystem->getSignaller(), &TestDbAddSignaller::sigComponentAdded);

    QCOMPARE(spyDbEntitiesAdded.count(), 0);
    QCOMPARE(spyDbComponentsAdded.count(), 0);
}

void test_mockandsqlitedatabase::logInsertsEntityComponents()
{
    m_testSystem->setupServer();
    m_testSystem->loadDatabase();
    m_testSystem->setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "ZeraAll");
    QSignalSpy spyDbEntitiesAdded(m_testSystem->getSignaller(), &TestDbAddSignaller::sigEntityAdded);
    QSignalSpy spyDbComponentsAdded(m_testSystem->getSignaller(), &TestDbAddSignaller::sigComponentAdded);

    m_testSystem->setComponentValues(1);
    QCOMPARE(spyDbEntitiesAdded.count(), 0);
    QCOMPARE(spyDbComponentsAdded.count(), 0);

    m_testSystem->startLogging();
    QFile file(":/sqlite-inserts/dumpRecordTwoEntityComponents.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = TestInsertSpiesToJson::spiesToJsonAndClear(spyDbEntitiesAdded, spyDbComponentsAdded);
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));

    m_testSystem->setComponentValues(2);

    QCOMPARE(spyDbEntitiesAdded.count(), 0);
    QCOMPARE(spyDbComponentsAdded.count(), 0);
}

void test_mockandsqlitedatabase::openDatabaseErrorEarly()
{
    m_testSystem->setupServer();
    m_testSystem->setComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerSystem::DBNameOpenErrorEarly);

    QFile file(":/vein-dumps/dumpDbOpenErrorEarly.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem->dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_mockandsqlitedatabase::openDatabaseOk()
{
    m_testSystem->setupServer();
    m_testSystem->loadDatabase();

    QJsonObject jsonDumped = QJsonDocument::fromJson(m_testSystem->dumpStorage()).object();
    QJsonObject loggerEntityDump = jsonDumped.value("2").toObject();
    QCOMPARE(loggerEntityDump.value("DatabaseReady").toBool(), true);
    QCOMPARE(loggerEntityDump.value("DatabaseFile").toString(), TestLoggerSystem::DBNameOpenOk);
}

void test_mockandsqlitedatabase::displaySessionInfo()
{
    QString sessionName = "Session1";
    QString transactionName = "Transaction1";

    m_testSystem->setupServer();
    m_testSystem->loadDatabase();
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", sessionName);
    m_testSystem->setComponent(dataLoggerEntityId, "guiContext", "ZeraGuiActualValues");
    m_testSystem->setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "ZeraAll");
    m_testSystem->startLogging("Session1", transactionName);
    m_testSystem->stopLogging();

    QFile file(":/sqlite-inserts/dumpSessionInfos.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QJsonObject jsonObj = m_testSystem->displaySessionsInfos(sessionName);
    //Remove 'Time' information as it indicates the actual time when logging was done. So it will never match with 'Time' from jsonExpected.
    QJsonObject temp = jsonObj.value(transactionName).toObject();
    temp.insert("Time", QString());
    jsonObj.insert(transactionName, temp);

    QJsonDocument jsonDoc(jsonObj);
    QString jsonDumped = jsonDoc.toJson(QJsonDocument::Indented);
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_mockandsqlitedatabase::displaySessionInfosInvalidSession()
{
    m_testSystem->setupServer();
    m_testSystem->loadDatabase();
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");
    m_testSystem->setComponent(dataLoggerEntityId, "guiContext", "ZeraGuiActualValues");
    m_testSystem->setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "ZeraAll");
    m_testSystem->startLogging("DbTestSession1", "Transaction1");
    m_testSystem->stopLogging();

    QJsonObject sessionInfo = m_testSystem->displaySessionsInfos("NonExistingSession");
    QVERIFY(sessionInfo.isEmpty());
}

void test_mockandsqlitedatabase::displaySessionInfosMultipleTransactions()
{
    m_testSystem->setupServer();
    m_testSystem->loadDatabase();
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");
    m_testSystem->setComponent(dataLoggerEntityId, "guiContext", "ZeraGuiActualValues");
    m_testSystem->setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "ZeraAll");
    m_testSystem->startLogging("DbTestSession1", "Transaction1");
    m_testSystem->stopLogging();
    m_testSystem->startLogging("DbTestSession1", "Transaction2");
    m_testSystem->stopLogging();
    m_testSystem->startLogging("DbTestSession1", "Transaction3");
    m_testSystem->stopLogging();

    QFile fileSessionInfo(":/session-infos/DbTestSession1MultipleTransactions.json");
    QVERIFY(fileSessionInfo.open(QFile::ReadOnly));
    QByteArray jsonExpected = fileSessionInfo.readAll();

    QJsonObject sessionInfo = m_testSystem->displaySessionsInfos("DbTestSession1");
    //Remove 'Time' information as it indicates the actual time when logging was done. So it will never match with 'Time' from jsonExpected.
    removeTimeInfoInTransactions(sessionInfo);

    QJsonDocument jsonDoc(sessionInfo);
    QString jsonDumped = jsonDoc.toJson(QJsonDocument::Indented);
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_mockandsqlitedatabase::deleteTransaction()
{
    m_testSystem->setupServer();
    m_testSystem->loadDatabase();
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");
    m_testSystem->setComponent(dataLoggerEntityId, "guiContext", "ZeraGuiActualValues");
    m_testSystem->setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "ZeraAll");
    m_testSystem->startLogging("DbTestSession1", "Transaction1");
    m_testSystem->stopLogging();
    m_testSystem->startLogging("DbTestSession1", "Transaction2");
    m_testSystem->stopLogging();

    QVERIFY(m_testSystem->deleteTransaction("Transaction1"));

    QJsonObject sessionInfo = m_testSystem->displaySessionsInfos("DbTestSession1");
    QVERIFY(!sessionInfo.contains("Transaction1"));
    QVERIFY(sessionInfo.contains("Transaction2"));
}

void test_mockandsqlitedatabase::deleteNonexistingTransaction()
{
    m_testSystem->setupServer();
    m_testSystem->loadDatabase();
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");
    m_testSystem->setComponent(dataLoggerEntityId, "guiContext", "ZeraGuiActualValues");
    m_testSystem->setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "ZeraAll");
    m_testSystem->startLogging("DbTestSession1", "Transaction1");
    m_testSystem->stopLogging();

    QVERIFY(!m_testSystem->deleteTransaction("foo"));
}

void test_mockandsqlitedatabase::deleteSession()
{
    m_testSystem->setupServer();
    m_testSystem->loadDatabase();
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");
    m_testSystem->startLogging("DbTestSession1", "Transaction1");
    m_testSystem->stopLogging();

    QJsonObject sessionInfo = m_testSystem->displaySessionsInfos("DbTestSession1");
    QVERIFY(!sessionInfo.isEmpty());
    QVERIFY(m_testSystem->deleteSession("DbTestSession1"));
    sessionInfo = m_testSystem->displaySessionsInfos("DbTestSession1");
    QVERIFY(sessionInfo.isEmpty());
}

void test_mockandsqlitedatabase::deleteNonexistingSession()
{
    m_testSystem->setupServer();
    m_testSystem->loadDatabase();

    QVERIFY(!m_testSystem->deleteSession("foo"));
}

void test_mockandsqlitedatabase::removeTimeInfoInTransactions(QJsonObject &sessionInfo)
{
    for(QString transaction: sessionInfo.keys()) {
        QJsonObject temp = sessionInfo.value(transaction).toObject();
        temp.insert("Time", QString());
        sessionInfo.insert(transaction, temp);
    }
}

void test_mockandsqlitedatabase::getAllSessions()
{
    m_testSystem->setupServer();
    m_testSystem->loadDatabase();
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "DbTestSession2");

    QFile fileSession(":/session-infos/AllSessions.json");
    QVERIFY(fileSession.open(QFile::ReadOnly));
    QByteArray jsonExpected = fileSession.readAll();

    QJsonArray allSessions = m_testSystem->getAllSessions();
    QJsonDocument jsonDoc(allSessions);
    QString jsonDumped = jsonDoc.toJson(QJsonDocument::Indented);
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_mockandsqlitedatabase::getNoSession()
{
    m_testSystem->setupServer();
    m_testSystem->loadDatabase();
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "DbTestSession2");

    QVERIFY(m_testSystem->deleteSession("DbTestSession1"));
    QVERIFY(m_testSystem->deleteSession("DbTestSession2"));

    QJsonArray allSessions = m_testSystem->getAllSessions();
    QVERIFY(allSessions.isEmpty());
}
