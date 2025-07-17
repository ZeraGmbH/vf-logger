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
    connect(m_dbLogger, &VeinLogger::DatabaseLogger::sigOpenDatabase,
            this, &RpcDisplayActualValues::onOpenDatabase);
}

void RpcDisplayActualValues::onOpenDatabase()
{
    connect(m_dbLogger->getDb(), &VeinLogger::AbstractLoggerDB::sigDisplayActualValuesCompleted,
            this, &RpcDisplayActualValues::onDisplayActualValuesCompleted, Qt::QueuedConnection);
}

void RpcDisplayActualValues::callRPCFunction(const QUuid &callId, const QVariantMap &parameters)
{
    RPC_displayActualValues(callId, parameters);
}

void RpcDisplayActualValues::onDisplayActualValuesCompleted(QUuid callId, bool success, QString errorMsg, QJsonObject values)
{
    if(success)
        sendRpcResult(callId, values.toVariantMap());
    else
        sendRpcError(callId, errorMsg);
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
