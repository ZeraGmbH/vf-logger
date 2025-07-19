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
    connect(m_dbLogger, &VeinLogger::DatabaseLogger::sigOpenDatabase,
            this, &RpcDisplaySessionsInfos::onOpenDatabase);
}

void RpcDisplaySessionsInfos::onOpenDatabase()
{
    connect(m_dbLogger->getDb().get(), &VeinLogger::AbstractLoggerDB::sigDisplaySessionInfosCompleted,
            this, &RpcDisplaySessionsInfos::onDisplaySessionInfosCompleted, Qt::QueuedConnection);
}

void RpcDisplaySessionsInfos::callRPCFunction(const QUuid &callId, const QVariantMap &parameters)
{
    if(m_dbLogger->isDatabaseReady()) {
        QString session = parameters["p_session"].toString();
        m_dbLogger->getDb()->startDisplaySessionsInfos(callId, session);
    }
    else
        sendRpcError(callId, "Database is not set");
}

void RpcDisplaySessionsInfos::onDisplaySessionInfosCompleted(QUuid callId, bool success, QString errorMsg, QJsonObject infos)
{
    if(success)
        sendRpcResult(callId, infos.toVariantMap());
    else
        sendRpcError(callId, errorMsg);
}
