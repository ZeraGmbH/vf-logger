#include "test_mockandsqlitedatabase.h"
#include "taskdbaddsession.h"
#include "testloghelpers.h"
#include "testinsertspiestojson.h"
#include "loggerstatictexts.h"
#include "svgfuzzycompare.h"
#include <timemachineobject.h>
#include <QTest>

Q_DECLARE_METATYPE(TestLoggerSystem::DbType)
QTEST_MAIN(test_mockandsqlitedatabase)

const char* db_temp_path = "/tmp/veindb-test";

void test_mockandsqlitedatabase::initTestCase_data()
{
    QTest::addColumn<TestLoggerSystem::DbType>("DbType");
    QTest::newRow("Mock Database") << TestLoggerSystem::DbType::MOCK;
    QTest::newRow("SQLite Database") << TestLoggerSystem::DbType::SQLITE;
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
    QDir dir(db_temp_path);
    dir.removeRecursively();
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
    QVERIFY(TestLogHelpers::compareAndLogOnDiffJson(jsonExpected, jsonDumped));
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
    m_testSystem->setComponent(dataLoggerEntityId, LoggerStaticTexts::s_currentContentSetsComponentName, QVariantList() << "ZeraAll");
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
    QVERIFY(TestLogHelpers::compareAndLogOnDiffJson(jsonExpected, jsonDumped));

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
    QVERIFY(TestLogHelpers::compareAndLogOnDiffJson(jsonExpected, jsonDumped));
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
    QVERIFY(TestLogHelpers::compareAndLogOnDiffJson(jsonExpected, jsonDumped));
}

void test_mockandsqlitedatabase::createTwoDatabases()
{
    m_testSystem->setComponent(dataLoggerEntityId, "DatabaseFile", QString(db_temp_path) + "/testDatabase1.db");
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "no customer");
    QJsonObject jsonDumped = QJsonDocument::fromJson(m_testSystem->dumpStorage()).object();
    QJsonObject loggerEntityDump = jsonDumped.value("2").toObject();
    QCOMPARE(loggerEntityDump.value("DatabaseReady").toBool(), true);
    QCOMPARE(loggerEntityDump.value("LoggingStatus").toString(), "Database loaded");

    m_testSystem->setComponent(dataLoggerEntityId, "DatabaseFile", QString(db_temp_path) + "/testDatabase2.db");
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "no customer");
    jsonDumped = QJsonDocument::fromJson(m_testSystem->dumpStorage()).object();
    loggerEntityDump = jsonDumped.value("2").toObject();
    QCOMPARE(loggerEntityDump.value("DatabaseReady").toBool(), true);
    QCOMPARE(loggerEntityDump.value("LoggingStatus").toString(), "Database loaded");
}

constexpr bool rpc_signature_ok = true;

void test_mockandsqlitedatabase::displaySessionInfo()
{
    m_testSystem->loadDatabase();
    logATransaction("Session1", "Transaction1", QStringList() << "ZeraAll");

    QVariantMap rpcParams;
    rpcParams.insert("p_session", "Session1");
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_displaySessionsInfos", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QCOMPARE(invokerSpy[0][0].toBool(), rpc_signature_ok);
    QCOMPARE(getResultCode(invokerSpy[0][1]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_SUCCESS);
    QJsonObject sessionInfo = getReturnedResult(invokerSpy[0][1]).toJsonObject();
    //Remove 'Time' information as it indicates the actual time when logging was done. So it will never match with 'Time' from jsonExpected.
    removeTimeInfoInTransactions(sessionInfo);
    QJsonDocument jsonDoc(sessionInfo);
    QString jsonDumped = jsonDoc.toJson(QJsonDocument::Indented);

    QFile file(":/sqlite-inserts/dumpSessionInfos.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QVERIFY(TestLogHelpers::compareAndLogOnDiffJson(jsonExpected, jsonDumped));
}

void test_mockandsqlitedatabase::displaySessionInfosInvalidSession()
{
    m_testSystem->loadDatabase();
    logATransaction("DbTestSession1", "Transaction1", QStringList() << "ZeraAll");

    QVariantMap rpcParams;
    rpcParams.insert("p_session", "NonExistingSession");
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_displaySessionsInfos", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QCOMPARE(invokerSpy[0][0].toBool(), rpc_signature_ok);
    QCOMPARE(getResultCode(invokerSpy[0][1]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_EINVAL);
    QCOMPARE(getErrorString(invokerSpy[0][1]), "Select an existing session");
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
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_displaySessionsInfos", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QCOMPARE(invokerSpy[0][0].toBool(), rpc_signature_ok);
    QCOMPARE(getResultCode(invokerSpy[0][1]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_SUCCESS);
    QJsonObject sessionInfo = getReturnedResult(invokerSpy[0][1]).toJsonObject();
    //Remove 'Time' information as it indicates the actual time when logging was done. So it will never match with 'Time' from jsonExpected.
    removeTimeInfoInTransactions(sessionInfo);
    QJsonDocument jsonDoc(sessionInfo);
    QString jsonDumped = jsonDoc.toJson(QJsonDocument::Indented);

    QFile fileSessionInfo(":/session-infos/DbTestSession1MultipleTransactions.json");
    QVERIFY(fileSessionInfo.open(QFile::ReadOnly));
    QByteArray jsonExpected = fileSessionInfo.readAll();

    QVERIFY(TestLogHelpers::compareAndLogOnDiffJson(jsonExpected, jsonDumped));
}

void test_mockandsqlitedatabase::displaySessionInfoBeforeDbLoaded()
{
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");

    QVariantMap rpcParams;
    rpcParams.insert("p_session", "DbTestSession1");
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_displaySessionsInfos", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QCOMPARE(invokerSpy[0][0].toBool(), rpc_signature_ok);
    QCOMPARE(getResultCode(invokerSpy[0][1]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_EINVAL);
    QCOMPARE(getErrorString(invokerSpy[0][1]), "Database is not set");
}

void test_mockandsqlitedatabase::deleteTransaction()
{
    m_testSystem->loadDatabase();
    logATransaction("DbTestSession1", "Transaction1", QStringList() << "ZeraAll");
    logATransaction("DbTestSession1", "Transaction2", QStringList() << "ZeraAll");

    QVariantMap rpcParams;
    rpcParams.insert("p_transaction", "Transaction1");
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_deleteTransaction", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QCOMPARE(invokerSpy[0][0].toBool(), rpc_signature_ok);
    QCOMPARE(getResultCode(invokerSpy[0][1]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_SUCCESS);
    QVERIFY(getReturnedResult(invokerSpy[0][1]).toBool());

    rpcParams.clear();
    rpcParams.insert("p_session", "DbTestSession1");
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_displaySessionsInfos", rpcParams);
    QJsonObject sessionInfo = invokerSpy[1][1].toMap().value(VeinComponent::RemoteProcedureData::s_returnString).toJsonObject();
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
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_deleteTransaction", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QCOMPARE(invokerSpy[0][0].toBool(), rpc_signature_ok);
    QCOMPARE(getResultCode(invokerSpy[0][1]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_EINVAL);
    QCOMPARE(getErrorString(invokerSpy[0][1]), "Select an existing transaction");
}

void test_mockandsqlitedatabase::deleteTransactionBeforeDbLoaded()
{
    logATransaction("DbTestSession1", "Transaction1", QStringList() << "ZeraAll");

    QVariantMap rpcParams;
    rpcParams.insert("p_transaction", "Transaction1");
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_deleteTransaction", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QCOMPARE(invokerSpy[0][0].toBool(), rpc_signature_ok);
    QCOMPARE(getResultCode(invokerSpy[0][1]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_EINVAL);
    QCOMPARE(getErrorString(invokerSpy[0][1]), "Database is not set");
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
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_deleteSession", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QCOMPARE(invokerSpy[0][0].toBool(), rpc_signature_ok);
    QCOMPARE(getResultCode(invokerSpy[0][1]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_SUCCESS);
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
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_deleteSession", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QCOMPARE(invokerSpy[0][0].toBool(), rpc_signature_ok);
    QCOMPARE(getResultCode(invokerSpy[0][1]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_EINVAL);
    QCOMPARE(getErrorString(invokerSpy[0][1]), "Select an existing session");
}

void test_mockandsqlitedatabase::deleteSessionBeforeDbLoaded()
{
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");

    QVariantMap rpcParams;
    rpcParams.insert("p_session", "DbTestSession1");
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_deleteSession", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QCOMPARE(invokerSpy[0][0].toBool(), rpc_signature_ok);
    QCOMPARE(getResultCode(invokerSpy[0][1]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_EINVAL);
    QCOMPARE(getErrorString(invokerSpy[0][1]), "Database is not set");
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
    AbstractLoggerDBPtr loggerDb = m_testSystem->getDbLogger()->getDb();

    // Generate sessions by component/task to cover more
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "DbTestSession2");
    std::shared_ptr<int> sessionId = std::make_shared<int>(-1);
    TaskDbAddSession task3(loggerDb, m_testSystem->getStorage(), "DbTestSession3", sessionId);
    task3.start();
    TaskDbAddSession task4(loggerDb, m_testSystem->getStorage(), "DbTestSession4", sessionId);
    task4.start();
    TimeMachineObject::feedEventLoop();

    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_listAllSessions", QVariantMap());

    QCOMPARE(invokerSpy.count(), 1);
    QCOMPARE(invokerSpy[0][0].toBool(), rpc_signature_ok);
    QCOMPARE(getResultCode(invokerSpy[0][1]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_SUCCESS);
    QJsonArray allSessions = getReturnedResult(invokerSpy[0][1]).toJsonArray();
    QJsonDocument jsonDoc(allSessions);
    QString jsonDumped = jsonDoc.toJson(QJsonDocument::Indented);

    QFile fileSession(":/session-infos/AllSessions.json");
    QVERIFY(fileSession.open(QFile::ReadOnly));
    QByteArray jsonExpected = fileSession.readAll();

    QVERIFY(TestLogHelpers::compareAndLogOnDiffJson(jsonExpected, jsonDumped));
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

    QJsonArray allSessions = getReturnedResult(invokerSpy[0][1]).toJsonArray();
    QVERIFY(allSessions.isEmpty());
}

void test_mockandsqlitedatabase::listAllSessionsBeforeDbLoaded()
{
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");

    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_listAllSessions", QVariantMap());

    QCOMPARE(invokerSpy.count(), 1);
    QCOMPARE(invokerSpy[0][0].toBool(), rpc_signature_ok);
    QCOMPARE(getResultCode(invokerSpy[0][1]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_EINVAL);
    QCOMPARE(getErrorString(invokerSpy[0][1]), "Database is not set");
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
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_displayActualValues", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QCOMPARE(invokerSpy[0][0].toBool(), rpc_signature_ok);
    QCOMPARE(getResultCode(invokerSpy[0][1]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_SUCCESS);
    QJsonObject jsonObj = getReturnedResult(invokerSpy[0][1]).toJsonObject();
    QJsonDocument jsonDoc(jsonObj);
    QString jsonDumped = jsonDoc.toJson(QJsonDocument::Indented);

    QFile file(":/logged-values/LogTestSet1.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QVERIFY(TestLogHelpers::compareAndLogOnDiffJson(jsonExpected, jsonDumped));
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
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_displayActualValues", rpcParams);
    QCOMPARE(invokerSpy.count(), 1);
    QCOMPARE(invokerSpy[0][0].toBool(), rpc_signature_ok);
    QCOMPARE(getResultCode(invokerSpy[0][1]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_SUCCESS);
    QJsonObject jsonDumped = getReturnedResult(invokerSpy[0][1]).toJsonObject();

    QFile file(":/logged-values/LogTestSet2.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QJsonParseError parseError;
    QJsonDocument docActual = QJsonDocument::fromJson(file.readAll(), &parseError);
    QJsonObject jsonExpected = docActual.object();

    QVERIFY(compareAndLogOnDiffIgnoringEntity(jsonExpected, jsonDumped, "0"));
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
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_displayActualValues", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QJsonObject jsonObj = getReturnedResult(invokerSpy[0][1]).toJsonObject();
    QJsonDocument jsonDoc(jsonObj);
    QString jsonDumped = jsonDoc.toJson(QJsonDocument::Indented);

    QFile file(":/logged-values/LogAll.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QVERIFY(TestLogHelpers::compareAndLogOnDiffJson(jsonExpected, jsonDumped));
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
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_displayActualValues", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QCOMPARE(invokerSpy[0][0].toBool(), rpc_signature_ok);
    QCOMPARE(getResultCode(invokerSpy[0][1]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_EINVAL);
    QCOMPARE(getErrorString(invokerSpy[0][1]), "Select an existing transaction");
}

void test_mockandsqlitedatabase::displayLoggedValuesBeforeDbLoaded()
{
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "session1");
    m_testSystem->startLogging("Session1", "Transaction1");
    m_testSystem->stopLogging();

    QVariantMap rpcParams;
    rpcParams.insert("p_transaction", "Transaction1");
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_displayActualValues", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QCOMPARE(invokerSpy[0][0].toBool(), rpc_signature_ok);
    QCOMPARE(getResultCode(invokerSpy[0][1]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_EINVAL);
    QCOMPARE(getErrorString(invokerSpy[0][1]), "Database is not set");
}

void test_mockandsqlitedatabase::NoCustomerDataSet()
{
    m_testSystem->appendCustomerDataSystem();
    m_testSystem->loadDatabase();
    QMap<int, QList<QString>> map = m_testSystem->getServer()->getTestEntityComponentInfo();
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "session1");

    QVariantMap rpcParams;
    rpcParams.insert("p_session", "session1");
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_displayCustomerData", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QCOMPARE(invokerSpy[0][0].toBool(), rpc_signature_ok);
    QCOMPARE(getResultCode(invokerSpy[0][1]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_SUCCESS);

    QJsonObject jsonObj = getReturnedResult(invokerSpy[0][1]).toJsonObject();
    QJsonDocument jsonDocument(jsonObj);
    QString jsonDumped = jsonDocument.toJson(QJsonDocument::Indented);

    QFile customerfile(":/customerData/emptyCustomerData.json");
    QVERIFY(customerfile.open(QFile::ReadOnly));
    QByteArray jsonExpected = customerfile.readAll();
    QVERIFY(TestLogHelpers::compareAndLogOnDiffJson(jsonExpected, jsonDumped));
}

void test_mockandsqlitedatabase::displayEmptyCustomerData()
{
    m_testSystem->appendCustomerDataSystem();
    m_testSystem->loadDatabase();

    QString dirPath = m_testSystem->getCustomerDataPath();
    QString filePath = QDir(dirPath).filePath("test.json");
    QJsonDocument jsonDoc;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
        qCritical() << "Could not open file for writing:" << filePath;

    file.write(jsonDoc.toJson());
    file.close();

    m_testSystem->setComponent(customerDataEntityId, "FileSelected", "test.json");
    TimeMachineObject::feedEventLoop();

    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "sessionTest");

    QVariantMap rpcParams;
    rpcParams.insert("p_session", "sessionTest");
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_displayCustomerData", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);

    QJsonObject jsonObj = getReturnedResult(invokerSpy[0][1]).toJsonObject();
    QJsonDocument jsonDocument(jsonObj);
    QString jsonDumped = jsonDocument.toJson(QJsonDocument::Indented);

    QFile customerfile(":/customerData/emptyCustomerData.json");
    QVERIFY(customerfile.open(QFile::ReadOnly));
    QByteArray jsonExpected = customerfile.readAll();
    QVERIFY(TestLogHelpers::compareAndLogOnDiffJson(jsonExpected, jsonDumped));
}

void test_mockandsqlitedatabase::displayEnteredCustomerData()
{
    m_testSystem->appendCustomerDataSystem();
    m_testSystem->loadDatabase();

    QString dirPath = m_testSystem->getCustomerDataPath();
    QString filePath = QDir(dirPath).filePath("test.json");
    QJsonDocument jsonDoc;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
        qCritical() << "Could not open file for writing:" << filePath;

    file.write(jsonDoc.toJson());
    file.close();

    m_testSystem->setComponent(customerDataEntityId, "FileSelected", "test.json");
    TimeMachineObject::feedEventLoop();
    m_testSystem->setCustomerDataComponent("PAR_CustomerFirstName", "foo");
    m_testSystem->setCustomerDataComponent("PAR_CustomerLastName", "bar");
    m_testSystem->setCustomerDataComponent("PAR_CustomerCity", "Königswinter");
    m_testSystem->setCustomerDataComponent("PAR_CustomerCountry", "Germany");

    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "sessionTest");

    QVariantMap rpcParams;
    rpcParams.insert("p_session", "sessionTest");
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_displayCustomerData", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);

    QJsonObject jsonObj = getReturnedResult(invokerSpy[0][1]).toJsonObject();
    QJsonDocument jsonDocument(jsonObj);
    QString jsonDumped = jsonDocument.toJson(QJsonDocument::Indented);

    QFile customerfile(":/customerData/filledCustomerData.json");
    QVERIFY(customerfile.open(QFile::ReadOnly));
    QByteArray jsonExpected = customerfile.readAll();
    QVERIFY(TestLogHelpers::compareAndLogOnDiffJson(jsonExpected, jsonDumped));
}

void test_mockandsqlitedatabase::createVectorDiagramDefaultOptions()
{
    m_testSystem->loadDatabase();
    logATransaction("session1", "Transaction1", QStringList() << "Transaction1");

    QVariantMap rpcParams;
    rpcParams.insert("p_transaction", "Transaction1");
    rpcParams.insert("p_paintingOptions", QVariantMap());
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_createVectorDiagram", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QCOMPARE(invokerSpy[0][0].toBool(), rpc_signature_ok);
    QCOMPARE(getResultCode(invokerSpy[0][1]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_SUCCESS);

    QString dumped = getReturnedResult(invokerSpy[0][1]).toString();
    QString expected = TestLogHelpers::loadFile(":/vector-diagram-svgs/vectorDiagramWithDefaultOptionsNoDftValues.svg");
    SvgFuzzyCompare compare;
    bool ok = compare.compareXml(dumped, expected);
    if(!ok)
        TestLogHelpers::compareAndLogOnDiff(expected, dumped);
    QVERIFY(ok);
}

void test_mockandsqlitedatabase::createVectorDiagramInvalidTransaction()
{
    m_testSystem->loadDatabase();
    logATransaction("session1", "Transaction1", QStringList() << "Transaction1");

    QVariantMap rpcParams;
    rpcParams.insert("p_transaction", "foo");
    rpcParams.insert("p_paintingOptions", QVariantMap());
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_createVectorDiagram", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QCOMPARE(invokerSpy[0][0].toBool(), rpc_signature_ok);
    QCOMPARE(getResultCode(invokerSpy[0][1]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_EINVAL);
    QCOMPARE(getErrorString(invokerSpy[0][1]), "Select an existing transaction");
}

void test_mockandsqlitedatabase::createVectorDiagramBeforeDbLoaded()
{
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", "DbTestSession1");

    QVariantMap rpcParams;
    rpcParams.insert("p_transaction", "foo");
    rpcParams.insert("p_paintingOptions", QVariantMap());
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_createVectorDiagram", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QCOMPARE(invokerSpy[0][0].toBool(), rpc_signature_ok);
    QCOMPARE(getResultCode(invokerSpy[0][1]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_EINVAL);
    QCOMPARE(getErrorString(invokerSpy[0][1]), "Database is not set");
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

bool test_mockandsqlitedatabase::compareAndLogOnDiffIgnoringEntity(QJsonObject expected, QJsonObject dumped, QString entityToBeIgnored)
{
    QJsonObject filteredExpected, filteredDumped;
    for (auto it = dumped.begin(); it != dumped.end(); ++it) {
        if (it.key() != entityToBeIgnored)
            filteredDumped.insert(it.key(), it.value());
    }
    for (auto it = expected.begin(); it != expected.end(); ++it) {
        if (it.key() != entityToBeIgnored)
            filteredExpected.insert(it.key(), it.value());
    }
    QJsonDocument jsonDoc(filteredDumped);
    QString strFilteredDumped = jsonDoc.toJson(QJsonDocument::Indented);
    jsonDoc = QJsonDocument(filteredExpected);
    QString strFilteredExpected = jsonDoc.toJson(QJsonDocument::Indented);
    if(strFilteredDumped != strFilteredExpected) {
        qWarning("EntityID 0 is not compared !");
        qWarning("Expected:");
        jsonDoc = QJsonDocument(expected);
        QString strJsonExpected = jsonDoc.toJson(QJsonDocument::Indented);
        qInfo("%s", qPrintable(strJsonExpected));
        qWarning("Dumped:");
        jsonDoc = QJsonDocument(dumped);
        QString strJsonDumped= jsonDoc.toJson(QJsonDocument::Indented);
        qInfo("%s", qPrintable(strJsonDumped));
        return false;
    }
    return true;
}
