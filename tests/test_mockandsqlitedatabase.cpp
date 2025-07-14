#include "test_mockandsqlitedatabase.h"
#include "testloghelpers.h"
#include "testinsertspiestojson.h"
#include "loggerstatictexts.h"
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
    m_testSystem->setupServer();
}

void test_mockandsqlitedatabase::cleanup()
{
    m_testSystem->cleanup();
    m_testSystem.reset();
}

void test_mockandsqlitedatabase::createSessionInsertsEntityComponents()
{
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
    m_testSystem->setComponent(dataLoggerEntityId, "DatabaseFile", TestLoggerSystem::DBNameOpenErrorEarly);

    QFile file(":/vein-dumps/dumpDbOpenErrorEarly.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem->dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_mockandsqlitedatabase::openDatabaseOk()
{
    m_testSystem->loadDatabase();

    QJsonObject jsonDumped = QJsonDocument::fromJson(m_testSystem->dumpStorage()).object();
    QJsonObject loggerEntityDump = jsonDumped.value("2").toObject();
    QCOMPARE(loggerEntityDump.value("DatabaseReady").toBool(), true);
    QCOMPARE(loggerEntityDump.value("DatabaseFile").toString(), TestLoggerSystem::DBNameOpenOk);
}

void test_mockandsqlitedatabase::openDatabaseErrorLate()
{
    m_testSystem->setComponent(dataLoggerEntityId, "DatabaseFile", ":/database/readOnlyDatabase.db");

    QFile file(":/vein-dumps/dumpDbOpenErrorLate.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem->dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_mockandsqlitedatabase::displaySessionInfo()
{
    m_testSystem->loadDatabase();
    logATransaction("Session1", "Transaction1", QStringList() << "ZeraAll");

    QVariantMap rpcParams;
    rpcParams.insert("p_session", "Session1");
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    QUuid id = m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_displaySessionsInfos", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QVERIFY(invokerSpy[0][0].toBool());
    QCOMPARE(invokerSpy[0][1], id);
    QCOMPARE(getResultCode(invokerSpy[0][2]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_SUCCESS);
    QJsonObject sessionInfo = getReturnedResult(invokerSpy[0][2]).toJsonObject();
    //Remove 'Time' information as it indicates the actual time when logging was done. So it will never match with 'Time' from jsonExpected.
    removeTimeInfoInTransactions(sessionInfo);
    QJsonDocument jsonDoc(sessionInfo);
    QString jsonDumped = jsonDoc.toJson(QJsonDocument::Indented);

    QFile file(":/sqlite-inserts/dumpSessionInfos.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_mockandsqlitedatabase::displaySessionInfosInvalidSession()
{
    m_testSystem->loadDatabase();
    logATransaction("DbTestSession1", "Transaction1", QStringList() << "ZeraAll");

    QVariantMap rpcParams;
    rpcParams.insert("p_session", "NonExistingSession");
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    QUuid id = m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_displaySessionsInfos", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QVERIFY(invokerSpy[0][0].toBool());
    QCOMPARE(invokerSpy[0][1], id);
    QCOMPARE(getResultCode(invokerSpy[0][2]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_EINVAL);
    QCOMPARE(getErrorString(invokerSpy[0][2]), "Select an existing session");
}

void test_mockandsqlitedatabase::displaySessionInfosMultipleTransactions()
{
    m_testSystem->loadDatabase();
    logATransaction("DbTestSession1", "Transaction1", QStringList() << "ZeraAll");
    logATransaction("DbTestSession1", "Transaction2", QStringList() << "ZeraAll");
    logATransaction("DbTestSession1", "Transaction3", QStringList() << "ZeraAll");

    QVariantMap rpcParams;
    rpcParams.insert("p_session", "DbTestSession1");
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    QUuid id = m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_displaySessionsInfos", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QVERIFY(invokerSpy[0][0].toBool());
    QCOMPARE(invokerSpy[0][1], id);
    QCOMPARE(getResultCode(invokerSpy[0][2]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_SUCCESS);
    QJsonObject sessionInfo = getReturnedResult(invokerSpy[0][2]).toJsonObject();
    //Remove 'Time' information as it indicates the actual time when logging was done. So it will never match with 'Time' from jsonExpected.
    removeTimeInfoInTransactions(sessionInfo);
    QJsonDocument jsonDoc(sessionInfo);
    QString jsonDumped = jsonDoc.toJson(QJsonDocument::Indented);

    QFile fileSessionInfo(":/session-infos/DbTestSession1MultipleTransactions.json");
    QVERIFY(fileSessionInfo.open(QFile::ReadOnly));
    QByteArray jsonExpected = fileSessionInfo.readAll();

    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_mockandsqlitedatabase::displaySessionInfoBeforeDbLoaded()
{
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");

    QVariantMap rpcParams;
    rpcParams.insert("p_session", "DbTestSession1");
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    QUuid id = m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_displaySessionsInfos", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QVERIFY(invokerSpy[0][0].toBool());
    QCOMPARE(invokerSpy[0][1], id);
    QCOMPARE(getResultCode(invokerSpy[0][2]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_EINVAL);
    QCOMPARE(getErrorString(invokerSpy[0][2]), "Database is not set");
}

void test_mockandsqlitedatabase::deleteTransaction()
{
    m_testSystem->loadDatabase();
    logATransaction("DbTestSession1", "Transaction1", QStringList() << "ZeraAll");
    logATransaction("DbTestSession1", "Transaction2", QStringList() << "ZeraAll");

    QVariantMap rpcParams;
    rpcParams.insert("p_transaction", "Transaction1");
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    QUuid id = m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_deleteTransaction", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QVERIFY(invokerSpy[0][0].toBool());
    QCOMPARE(invokerSpy[0][1], id);
    QCOMPARE(getResultCode(invokerSpy[0][2]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_SUCCESS);
    QVERIFY(getReturnedResult(invokerSpy[0][2]).toBool());

    rpcParams.clear();
    rpcParams.insert("p_session", "DbTestSession1");
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_displaySessionsInfos", rpcParams);
    QJsonObject sessionInfo = invokerSpy[1][2].toMap().value(VeinComponent::RemoteProcedureData::s_returnString).toJsonObject();
    QVERIFY(!sessionInfo.contains("Transaction1"));
    QVERIFY(sessionInfo.contains("Transaction2"));
}

void test_mockandsqlitedatabase::deleteNonexistingTransaction()
{
    m_testSystem->loadDatabase();
    logATransaction("DbTestSession1", "Transaction1", QStringList() << "ZeraAll");

    QVariantMap rpcParams;
    rpcParams.insert("p_transaction", "foo");
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    QUuid id = m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_deleteTransaction", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QVERIFY(invokerSpy[0][0].toBool());
    QCOMPARE(invokerSpy[0][1], id);
    QCOMPARE(getResultCode(invokerSpy[0][2]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_EINVAL);
    QCOMPARE(getErrorString(invokerSpy[0][2]), "Select an existing transaction");
}

void test_mockandsqlitedatabase::deleteTransactionBeforeDbLoaded()
{
    logATransaction("DbTestSession1", "Transaction1", QStringList() << "ZeraAll");

    QVariantMap rpcParams;
    rpcParams.insert("p_transaction", "Transaction1");
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    QUuid id = m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_deleteTransaction", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QVERIFY(invokerSpy[0][0].toBool());
    QCOMPARE(invokerSpy[0][1], id);
    QCOMPARE(getResultCode(invokerSpy[0][2]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_EINVAL);
    QCOMPARE(getErrorString(invokerSpy[0][2]), "Database is not set");
}

void test_mockandsqlitedatabase::deleteSession()
{
    m_testSystem->loadDatabase();
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");

    QStringList exisitingSessions = getComponentValue(dataLoggerEntityId, LoggerStaticTexts::s_existingSessionsComponentName).toStringList();
    QVERIFY(exisitingSessions.contains("DbTestSession1"));
    QString currentSession =  getComponentValue(dataLoggerEntityId, LoggerStaticTexts::s_sessionNameComponentName).toString();
    QCOMPARE(currentSession, "DbTestSession1");

    QVariantMap rpcParams;
    rpcParams.insert("p_session", "DbTestSession1");
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    QUuid id = m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_deleteSession", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QVERIFY(invokerSpy[0][0].toBool());
    QCOMPARE(invokerSpy[0][1], id);
    QCOMPARE(getResultCode(invokerSpy[0][2]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_SUCCESS);
    exisitingSessions = getComponentValue(dataLoggerEntityId, LoggerStaticTexts::s_existingSessionsComponentName).toStringList();
    QVERIFY(!exisitingSessions.contains("DbTestSession1"));
    currentSession =  getComponentValue(dataLoggerEntityId, LoggerStaticTexts::s_sessionNameComponentName).toString();
    QCOMPARE(currentSession, "");
}

void test_mockandsqlitedatabase::deleteNonexistingSession()
{
    m_testSystem->loadDatabase();

    QVariantMap rpcParams;
    rpcParams.insert("p_session", "foo");
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    QUuid id = m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_deleteSession", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QVERIFY(invokerSpy[0][0].toBool());
    QCOMPARE(invokerSpy[0][1], id);
    QCOMPARE(getResultCode(invokerSpy[0][2]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_EINVAL);
    QCOMPARE(getErrorString(invokerSpy[0][2]), "Select an existing session");
}

void test_mockandsqlitedatabase::deleteSessionBeforeDbLoaded()
{
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");

    QVariantMap rpcParams;
    rpcParams.insert("p_session", "DbTestSession1");
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    QUuid id = m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_deleteSession", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QVERIFY(invokerSpy[0][0].toBool());
    QCOMPARE(invokerSpy[0][1], id);
    QCOMPARE(getResultCode(invokerSpy[0][2]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_EINVAL);
    QCOMPARE(getErrorString(invokerSpy[0][2]), "Database is not set");
}

void test_mockandsqlitedatabase::removeTimeInfoInTransactions(QJsonObject &sessionInfo)
{
    for(QString transaction: sessionInfo.keys()) {
        QJsonObject temp = sessionInfo.value(transaction).toObject();
        temp.insert("Time", QString());
        sessionInfo.insert(transaction, temp);
    }
}

QVariant test_mockandsqlitedatabase::getComponentValue(int entityId, QString component)
{
    return m_testSystem->getServer()->getStorage()->getDb()->getStoredValue(entityId, component);
}

void test_mockandsqlitedatabase::listAllSessions()
{
    m_testSystem->loadDatabase();
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "DbTestSession2");

    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    QUuid id = m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_listAllSessions", QVariantMap());

    QCOMPARE(invokerSpy.count(), 1);
    QVERIFY(invokerSpy[0][0].toBool());
    QCOMPARE(invokerSpy[0][1], id);
    QCOMPARE(getResultCode(invokerSpy[0][2]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_SUCCESS);
    QJsonArray allSessions = getReturnedResult(invokerSpy[0][2]).toJsonArray();
    QJsonDocument jsonDoc(allSessions);
    QString jsonDumped = jsonDoc.toJson(QJsonDocument::Indented);

    QFile fileSession(":/session-infos/AllSessions.json");
    QVERIFY(fileSession.open(QFile::ReadOnly));
    QByteArray jsonExpected = fileSession.readAll();

    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_mockandsqlitedatabase::listNoSession()
{
    m_testSystem->loadDatabase();
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "DbTestSession2");

    QVariantMap rpcParams;
    rpcParams.insert("p_session", "DbTestSession1");
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_deleteSession", rpcParams);
    rpcParams.insert("p_session", "DbTestSession2");
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_deleteSession", rpcParams);

    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_listAllSessions", QVariantMap());

    QJsonArray allSessions = getReturnedResult(invokerSpy[0][2]).toJsonArray();
    QVERIFY(allSessions.isEmpty());
}

void test_mockandsqlitedatabase::listAllSessionsBeforeDbLoaded()
{
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");

    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    QUuid id = m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_listAllSessions", QVariantMap());

    QCOMPARE(invokerSpy.count(), 1);
    QVERIFY(invokerSpy[0][0].toBool());
    QCOMPARE(invokerSpy[0][1], id);
    QCOMPARE(getResultCode(invokerSpy[0][2]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_EINVAL);
    QCOMPARE(getErrorString(invokerSpy[0][2]), "Database is not set");
}

void test_mockandsqlitedatabase::displayLoggedValues()
{
    int testSet1EntityId = 10;
    int testSet2EntityId = 11;

    m_testSystem->setComponent(testSet1EntityId, "ComponentName1" , 1);
    m_testSystem->setComponent(testSet1EntityId, "ComponentName2" , 2);
    m_testSystem->setComponent(testSet2EntityId, "ComponentName1" , 1);
    m_testSystem->setComponent(testSet2EntityId, "ComponentName2" , 2);

    m_testSystem->loadDatabase();
    logATransaction("Session1", "Transaction1", QStringList() << "TestSet1");

    QVariantMap rpcParams;
    rpcParams.insert("p_transaction", "Transaction1");
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    QUuid id = m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_displayActualValues", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QVERIFY(invokerSpy[0][0].toBool());
    QCOMPARE(invokerSpy[0][1], id);
    QCOMPARE(getResultCode(invokerSpy[0][2]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_SUCCESS);
    QJsonObject jsonObj = getReturnedResult(invokerSpy[0][2]).toJsonObject();
    QJsonDocument jsonDoc(jsonObj);
    QString jsonDumped = jsonDoc.toJson(QJsonDocument::Indented);

    QFile file(":/logged-values/LogTestSet1.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_mockandsqlitedatabase::displayLoggedValuesOnDifferentSessionDeviceName()
{
    int testSet1EntityId = 10;
    int testSet2EntityId = 11;
    int entityAndComponentCount = 2;
    m_testSystem->setupServer(entityAndComponentCount , entityAndComponentCount , QList<int>() << systemEntityId);

    m_testSystem->setComponent(testSet1EntityId, "ComponentName1" , 1);
    m_testSystem->setComponent(testSet1EntityId, "ComponentName2" , 2);
    m_testSystem->setComponent(testSet2EntityId, "ComponentName1" , 1);
    m_testSystem->setComponent(testSet2EntityId, "ComponentName2" , 2);

    m_testSystem->loadDatabase();
    logATransaction("Session1", "Transaction1", QStringList() << "TestSet2");

    m_testSystem->changeSession();

    QVariantMap rpcParams;
    rpcParams.insert("p_transaction", "Transaction1");
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    QUuid id = m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_displayActualValues", rpcParams);
    QCOMPARE(invokerSpy.count(), 1);
    QVERIFY(invokerSpy[0][0].toBool());
    QCOMPARE(invokerSpy[0][1], id);
    QCOMPARE(getResultCode(invokerSpy[0][2]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_SUCCESS);
    QJsonObject jsonObj = getReturnedResult(invokerSpy[0][2]).toJsonObject();

    QJsonDocument jsonDoc(filterEntitySystem(jsonObj));
    QString jsonDumped = jsonDoc.toJson(QJsonDocument::Indented);

    QFile file(":/logged-values/LogTestSet2.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QJsonParseError parseError;
    QJsonDocument docActual = QJsonDocument::fromJson(file.readAll(), &parseError);

    QJsonObject filteredActual = filterEntitySystem(docActual.object());
    jsonDoc = QJsonDocument(filterEntitySystem(docActual.object()));
    QString jsonExpected = jsonDoc.toJson(QJsonDocument::Indented);

    // We ingore 'System' Entity in this comparison
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_mockandsqlitedatabase::displayLoggedValuesZeraAll()
{
    int testSet1EntityId = 10;
    int testSet2EntityId = 11;

    m_testSystem->setComponent(testSet1EntityId, "ComponentName1" , 1);
    m_testSystem->setComponent(testSet1EntityId, "ComponentName2" , 2);
    m_testSystem->setComponent(testSet2EntityId, "ComponentName1" , 1);
    m_testSystem->setComponent(testSet2EntityId, "ComponentName2" , 2);

    m_testSystem->loadDatabase();
    logATransaction("Session1", "Transaction1", QStringList() << "ZeraAll");

    QVariantMap rpcParams;
    rpcParams.insert("p_transaction", "Transaction1");
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    QUuid id = m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_displayActualValues", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QJsonObject jsonObj = getReturnedResult(invokerSpy[0][2]).toJsonObject();
    QJsonDocument jsonDoc(jsonObj);
    QString jsonDumped = jsonDoc.toJson(QJsonDocument::Indented);

    QFile file(":/logged-values/LogAll.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_mockandsqlitedatabase::displayLoggedValuesInvalidTransaction()
{
    int testSet1EntityId = 10;
    int testSet2EntityId = 11;

    m_testSystem->setComponent(testSet1EntityId, "ComponentName1" , 1);
    m_testSystem->setComponent(testSet1EntityId, "ComponentName2" , 2);
    m_testSystem->setComponent(testSet2EntityId, "ComponentName1" , 1);
    m_testSystem->setComponent(testSet2EntityId, "ComponentName2" , 2);

    m_testSystem->loadDatabase();
    logATransaction("session1", "Transaction1", QStringList() << "Transaction1");

    QVariantMap rpcParams;
    rpcParams.insert("p_transaction", "InvalidTransaction");
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    QUuid id = m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_displayActualValues", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QVERIFY(invokerSpy[0][0].toBool());
    QCOMPARE(invokerSpy[0][1], id);
    QCOMPARE(getResultCode(invokerSpy[0][2]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_EINVAL);
    QCOMPARE(getErrorString(invokerSpy[0][2]), "Select an existing transaction");
}

void test_mockandsqlitedatabase::displayLoggedValuesBeforeDbLoaded()
{
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "session1");
    m_testSystem->startLogging("Session1", "Transaction1");
    m_testSystem->stopLogging();

    QVariantMap rpcParams;
    rpcParams.insert("p_transaction", "Transaction1");
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    QUuid id = m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_displayActualValues", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QVERIFY(invokerSpy[0][0].toBool());
    QCOMPARE(invokerSpy[0][1], id);
    QCOMPARE(getResultCode(invokerSpy[0][2]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_EINVAL);
    QCOMPARE(getErrorString(invokerSpy[0][2]), "Database is not set");
}

void test_mockandsqlitedatabase::logATransaction(QString session, QString transaction, QStringList contentSets)
{
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", session);
    m_testSystem->setComponent(dataLoggerEntityId, "guiContext", "ZeraGuiActualValues");
    m_testSystem->setComponent(dataLoggerEntityId, "currentContentSets", contentSets);
    m_testSystem->startLogging(session, transaction);
    m_testSystem->stopLogging();
}

QVariant test_mockandsqlitedatabase::getResultCode(QVariant rpcReturnData)
{
    return rpcReturnData.toMap().value(VeinComponent::RemoteProcedureData::s_resultCodeString);
}

QString test_mockandsqlitedatabase::getErrorString(QVariant rpcReturnData)
{
    return rpcReturnData.toMap().value(VeinComponent::RemoteProcedureData::s_errorMessageString).toString();
}

QVariant test_mockandsqlitedatabase::getReturnedResult(QVariant rpcReturnData)
{
    return rpcReturnData.toMap().value(VeinComponent::RemoteProcedureData::s_returnString);
}

QJsonObject test_mockandsqlitedatabase::filterEntitySystem(QJsonObject obj)
{
    QJsonObject filtered;
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (it.key() == "0")
            continue; // Skip key "0"
        if (it.value().isObject())
            filtered[it.key()] = filterEntitySystem(it.value().toObject());
    }
    return filtered;
}
