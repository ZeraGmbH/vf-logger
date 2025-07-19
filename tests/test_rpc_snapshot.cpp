#include "test_rpc_snapshot.h"
#include <QSignalSpy>
#include <QTest>

QTEST_MAIN(test_rpc_snapshot)

void test_rpc_snapshot::init()
{
    m_testSystem = std::make_unique<TestLoggerSystem>(TestLoggerSystem::DbType::MOCK);
    m_testSystem->setupServer();
}

void test_rpc_snapshot::cleanup()
{
    m_testSystem->cleanup();
    m_testSystem.reset();
}

constexpr bool rpc_signature_ok = true;

void test_rpc_snapshot::invalidParams()
{
    QVariantMap rpcParams;
    rpcParams.insert("p_foo", QStringList() << "foo");

    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    QUuid id = m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_createSnapshot", rpcParams);

    QCOMPARE(invokerSpy.count(), 1);
    QCOMPARE(invokerSpy[0][0].toBool(), !rpc_signature_ok);
    QCOMPARE(invokerSpy[0][1], id);
    // Default returns invocation data in result
}

void test_rpc_snapshot::noDatabaseLoaded()
{
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    QUuid id = m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_createSnapshot", getValidRpcParams());

    QCOMPARE(invokerSpy.count(), 1);
    QCOMPARE(invokerSpy[0][0].toBool(), rpc_signature_ok);
    QCOMPARE(invokerSpy[0][1], id);
    QCOMPARE(getResultCode(invokerSpy[0][2]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_EINVAL);
    const QStringList errStr = getErrorString(invokerSpy[0][2]).split("\n");
    // header + no database
    QCOMPARE(errStr.count(), 2);
}

void test_rpc_snapshot::noSessionSet()
{
    m_testSystem->loadDatabase();

    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    QVariantMap params = getValidRpcParams();
    params["p_sessionName"] = QString();
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_createSnapshot", params);

    QCOMPARE(invokerSpy.count(), 1);
    QCOMPARE(invokerSpy[0][0].toBool(), rpc_signature_ok);
    QCOMPARE(getResultCode(invokerSpy[0][2]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_EINVAL);
    const QStringList errStr = getErrorString(invokerSpy[0][2]).split("\n");
    // header + no session
    QCOMPARE(errStr.count(), 2);
}

void test_rpc_snapshot::noTransactionSet()
{
    m_testSystem->loadDatabase();

    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    QVariantMap params = getValidRpcParams();
    params["p_transactionName"] = QString();
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_createSnapshot", params);

    QCOMPARE(invokerSpy.count(), 1);
    QCOMPARE(invokerSpy[0][0].toBool(), rpc_signature_ok);
    QCOMPARE(getResultCode(invokerSpy[0][2]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_EINVAL);
    const QStringList errStr = getErrorString(invokerSpy[0][2]).split("\n");
    // header + no transaction
    QCOMPARE(errStr.count(), 2);
}

void test_rpc_snapshot::emptyContentSet()
{
    m_testSystem->loadDatabase();

    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    QVariantMap params = getValidRpcParams();
    params["p_contentSets"] = QStringList();
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_createSnapshot", params);

    QCOMPARE(invokerSpy.count(), 1);
    QCOMPARE(invokerSpy[0][0].toBool(), rpc_signature_ok);
    QCOMPARE(getResultCode(invokerSpy[0][2]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_EINVAL);
    const QStringList errStr = getErrorString(invokerSpy[0][2]).split("\n");
    // header + empty transaction
    QCOMPARE(errStr.count(), 2);
}

static const char* noError = "";

void test_rpc_snapshot::wip()
{
    m_testSystem->loadDatabase();

    QSignalSpy spyTransactionAdded(m_testSystem->getSignaller(), &TestDbAddSignaller::sigAddTransaction);
    QSignalSpy invokerSpy(m_testSystem->getServer(), &TestVeinServer::sigRPCFinished);
    m_testSystem->getServer()->invokeRpc(dataLoggerEntityId, "RPC_createSnapshot", getValidRpcParams());

    QCOMPARE(invokerSpy.count(), 1);
    QCOMPARE(invokerSpy[0][0].toBool(), rpc_signature_ok);
    QCOMPARE(getResultCode(invokerSpy[0][2]), VeinComponent::RemoteProcedureData::RPCResultCodes::RPC_SUCCESS);
    QCOMPARE(invokerSpy[0][2], noError);


}

QVariantMap test_rpc_snapshot::createRpcParams(const QString &transactionName,
                                               const QString &sessionName,
                                               const QStringList &contentSets,
                                               const QString &guiContextName)
{
    QVariantMap rpcParams;
    rpcParams.insert("p_transactionName", transactionName);
    rpcParams.insert("p_sessionName", sessionName);
    rpcParams.insert("p_contentSets", contentSets);
    rpcParams.insert("p_guiContext", guiContextName);
    return rpcParams;
}

QVariantMap test_rpc_snapshot::getValidRpcParams()
{
    return createRpcParams("testTransaction",
                           "testSessionName",
                           QStringList() << "ZeraAll",
                           "ZeraGuiActualValues");
}

QVariant test_rpc_snapshot::getResultCode(QVariant rpcReturnData)
{
    return rpcReturnData.toMap().value(VeinComponent::RemoteProcedureData::s_resultCodeString);
}

QString test_rpc_snapshot::getErrorString(QVariant rpcReturnData)
{
    return rpcReturnData.toMap().value(VeinComponent::RemoteProcedureData::s_errorMessageString).toString();
}
