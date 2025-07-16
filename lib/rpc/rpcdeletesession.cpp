#include "rpcdeletesession.h"
#include "vl_databaselogger.h"
#include <vf-cpp-rpc-signature.h>

RpcDeleteSession::RpcDeleteSession(VeinLogger::DatabaseLogger *dbLogger,
                                   int entityId) :
    VfCpp::VfCppRpcSimplified(dbLogger,
                              entityId,
                              VfCpp::VfCppRpcSignature::createRpcSignature(
                                  "RPC_deleteSession",
                                  VfCpp::VfCppRpcSignature::RPCParams({{"p_session", "QString"}}))),
    m_dbLogger(dbLogger)
{
}

void RpcDeleteSession::callRPCFunction(const QUuid &callId, const QVariantMap &parameters)
{
    RPC_deleteSession(callId, parameters);
}

void RpcDeleteSession::RPC_deleteSession(QUuid callId, QVariantMap parameters)
{
    if(m_dbLogger->isDatabaseReady()) {
        QString session = parameters["p_session"].toString();
        emit m_dbLogger->getDb()->startDeleteSession(callId, session);
    }
    else
        sendRpcError(callId, "Database is not set");
}
