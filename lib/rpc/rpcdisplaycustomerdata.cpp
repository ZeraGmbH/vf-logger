#include "rpcdisplaycustomerdata.h"
#include "vl_databaselogger.h"
#include <vf-cpp-rpc-signature.h>

RpcDisplayCustomerData::RpcDisplayCustomerData(VeinLogger::DatabaseLogger *dbLogger, int entityId) :
    VfCpp::VfCppRpcSimplified(dbLogger,
                              entityId,
                              VfCpp::VfCppRpcSignature::createRpcSignature(
                                  "RPC_displayCustomerData",
                                  VfCpp::VfCppRpcSignature::RPCParams({{"p_session", "QString"}}))),
    m_dbLogger(dbLogger)
{
    connect(m_dbLogger, &VeinLogger::DatabaseLogger::sigOpenDatabase,
            this, &RpcDisplayCustomerData::onOpenDatabase);
}

void RpcDisplayCustomerData::onOpenDatabase()
{
    connect(m_dbLogger->getDb().get(), &VeinLogger::AbstractLoggerDB::sigDisplayCustomerDataCompleted,
            this, &RpcDisplayCustomerData::onDisplayCustomerDataCompleted, Qt::QueuedConnection);
}

void RpcDisplayCustomerData::callRPCFunction(const QUuid &callId, const QVariantMap &parameters)
{
    if(m_dbLogger->isDatabaseReady()) {
        QString session = parameters["p_session"].toString();
        m_dbLogger->getDb()->startDisplayCustomerData(callId, session);
    }
    else
        sendRpcError(callId, "Database is not set");
}

void RpcDisplayCustomerData::onDisplayCustomerDataCompleted(QUuid callId, bool success, QString errorMsg, QJsonObject custData)
{
    if(success)
        sendRpcResult(callId, custData.toVariantMap());
    else
        sendRpcError(callId, errorMsg);
}
