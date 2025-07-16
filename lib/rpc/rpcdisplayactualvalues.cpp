#include "rpcdisplayactualvalues.h"
#include "vl_databaselogger.h"
#include <vf-cpp-rpc-signature.h>

RpcDisplayActualValues::RpcDisplayActualValues(VeinLogger::DatabaseLogger *dbLogger,
                                               int entityId) :
    VfCpp::VfCppRpcSimplified(dbLogger,
                              entityId,
                              VfCpp::VfCppRpcSignature::createRpcSignature(
                                  "RPC_displayActualValues",
                                  VfCpp::VfCppRpcSignature::RPCParams({{"p_transaction", "QString"}}))),
    m_dbLogger(dbLogger)
{
}

void RpcDisplayActualValues::callRPCFunction(const QUuid &callId, const QVariantMap &parameters)
{
    RPC_displayActualValues(callId, parameters);
}

void RpcDisplayActualValues::RPC_displayActualValues(QUuid callId, QVariantMap parameters)
{
    if(m_dbLogger->isDatabaseReady()) {
        QString transaction = parameters["p_transaction"].toString();
        m_dbLogger->getDb()->startDisplayActualValues(callId, transaction);
    }
    else
        sendRpcError(callId, "Database is not set");
}
