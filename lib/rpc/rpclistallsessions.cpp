#include "rpclistallsessions.h"
#include "vl_databaselogger.h"
#include <vf-cpp-rpc-signature.h>

RpcListAllSessions::RpcListAllSessions(VeinLogger::DatabaseLogger *dbLogger,
                                       int entityId) :
    VfCpp::VfCppRpcSimplified(dbLogger,
                              entityId,
                              VfCpp::VfCppRpcSignature::createRpcSignature(
                                    "RPC_listAllSessions",
                                    VfCpp::VfCppRpcSignature::RPCParams({}))),
    m_dbLogger(dbLogger)
{
    connect(m_dbLogger, &VeinLogger::DatabaseLogger::sigOpenDatabase,
            this, &RpcListAllSessions::onOpenDatabase);
}

void RpcListAllSessions::onOpenDatabase()
{
    connect(m_dbLogger->getDb(), &VeinLogger::AbstractLoggerDB::sigListAllSessionsCompleted,
            this, &RpcListAllSessions::onListAllSessionsCompleted, Qt::QueuedConnection);
}

void RpcListAllSessions::callRPCFunction(const QUuid &callId, const QVariantMap &parameters)
{
    Q_UNUSED(parameters)
    if(m_dbLogger->isDatabaseReady())
        m_dbLogger->getDb()->startListAllSessions(callId);
    else
        sendRpcError(callId, "Database is not set");
}

void RpcListAllSessions::onListAllSessionsCompleted(QUuid callId, bool success, QString errorMsg, QJsonArray sessions)
{
    if(success)
        sendRpcResult(callId, sessions.toVariantList());
    else
        sendRpcError(callId, errorMsg);
}
