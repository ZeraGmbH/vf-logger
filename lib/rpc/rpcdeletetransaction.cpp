#include "rpcdeletetransaction.h"
#include "vl_databaselogger.h"
#include <vf-cpp-rpc-signature.h>

RpcDeleteTransaction::RpcDeleteTransaction(VeinLogger::DatabaseLogger *dbLogger,
                                           int entityId) :
    VfCpp::VfCppRpcSimplified(dbLogger,
                              entityId,
                              VfCpp::VfCppRpcSignature::createRpcSignature(
                                  "RPC_deleteTransaction",
                                  VfCpp::VfCppRpcSignature::RPCParams({{"p_transaction", "QString"}}))),
    m_dbLogger(dbLogger)
{
}

void RpcDeleteTransaction::callRPCFunction(const QUuid &callId, const QVariantMap &parameters)
{
    RPC_deleteTransaction(callId, parameters);
}

void RpcDeleteTransaction::RPC_deleteTransaction(QUuid callId, QVariantMap parameters)
{
    if(m_dbLogger->isDatabaseReady()) {
        QString transaction = parameters["p_transaction"].toString();
        m_dbLogger->getDb()->startDeleteTransaction(callId, transaction);
    }
    else
        sendRpcError(callId, "Database is not set");
}
