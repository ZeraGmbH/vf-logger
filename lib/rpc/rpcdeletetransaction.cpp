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
    connect(m_dbLogger, &VeinLogger::DatabaseLogger::sigOpenDatabase,
            this, &RpcDeleteTransaction::onOpenDatabase);
}

void RpcDeleteTransaction::onOpenDatabase()
{
    connect(m_dbLogger->getDb(), &VeinLogger::AbstractLoggerDB::sigDeleteTransactionCompleted,
            this, &RpcDeleteTransaction::onDeleteTransactionCompleted, Qt::QueuedConnection);
}

void RpcDeleteTransaction::callRPCFunction(const QUuid &callId, const QVariantMap &parameters)
{
    RPC_deleteTransaction(callId, parameters);
}

void RpcDeleteTransaction::onDeleteTransactionCompleted(QUuid callId, bool success, QString errorMsg)
{
    if(success)
        sendRpcResult(callId, true);
    else
        sendRpcError(callId, errorMsg);
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
