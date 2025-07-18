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
    connect(m_dbLogger, &VeinLogger::DatabaseLogger::sigDeleteSessionCompleted,
            this, &RpcDeleteSession::onDeleteSessionCompleted, Qt::QueuedConnection);
}

void RpcDeleteSession::callRPCFunction(const QUuid &callId, const QVariantMap &parameters)
{
    if(m_dbLogger->isDatabaseReady()) {
        QString session = parameters["p_session"].toString();
        m_dbLogger->getDb()->startDeleteSession(callId, session);
    }
    else
        sendRpcError(callId, "Database is not set");
}

void RpcDeleteSession::onDeleteSessionCompleted(QUuid callId, bool success, QString errorMsg)
{
    if(success)
        sendRpcResult(callId, true);
    else
        sendRpcError(callId, errorMsg);
}

