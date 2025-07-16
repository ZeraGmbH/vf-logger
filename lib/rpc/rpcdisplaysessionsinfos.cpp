#include "rpcdisplaysessionsinfos.h"
#include "vl_databaselogger.h"
#include <vf-cpp-rpc-signature.h>

RpcDisplaySessionsInfos::RpcDisplaySessionsInfos(VeinLogger::DatabaseLogger *dbLogger, int entityId) :
    VfCpp::VfCppRpcSimplified(dbLogger,
                              entityId,
                              VfCpp::VfCppRpcSignature::createRpcSignature(
                                  "RPC_displaySessionsInfos",
                                  VfCpp::VfCppRpcSignature::RPCParams({{"p_session", "QString"}}))),
    m_dbLogger(dbLogger)
{
}

void RpcDisplaySessionsInfos::callRPCFunction(const QUuid &callId, const QVariantMap &parameters)
{
    RPC_displaySessionsInfos(callId, parameters);
}

void RpcDisplaySessionsInfos::RPC_displaySessionsInfos(QUuid callId, QVariantMap parameters)
{
    if(m_dbLogger->isDatabaseReady()) {
        QString session = parameters["p_session"].toString();
        m_dbLogger->getDb()->startDisplaySessionsInfos(callId, session);
    }
    else
        sendRpcError(callId, "Database is not set");
}
