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
}

void RpcListAllSessions::callRPCFunction(const QUuid &callId, const QVariantMap &parameters)
{
    Q_UNUSED(parameters)
    RPC_listAllSessions(callId);
}

void RpcListAllSessions::RPC_listAllSessions(QUuid callId)
{
    if(m_dbLogger->isDatabaseReady())
        m_dbLogger->getDb()->startListAllSessions(callId);
    else
        sendRpcError(callId, "Database is not set");
}
