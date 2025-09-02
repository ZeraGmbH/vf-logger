#include "rpccreatevectordiagram.h"
#include "vl_databaselogger.h"
#include "vectordiagramcreator.h"
#include <vf-cpp-rpc-signature.h>

RpcCreateVectorDiagram::RpcCreateVectorDiagram(VeinLogger::DatabaseLogger *dbLogger, int entityId) :
    VfCpp::VfCppRpcSimplified(dbLogger,
                              entityId,
                              VfCpp::VfCppRpcSignature::createRpcSignature(
                                  "RPC_createVectorDiagram",
                                  VfCpp::VfCppRpcSignature::RPCParams({{"p_transaction", "QString"}, {"p_paintingOptions", "QString"}}))),
    m_dbLogger(dbLogger)
{
    connect(m_dbLogger, &VeinLogger::DatabaseLogger::sigOpenDatabase,
            this, &RpcCreateVectorDiagram::onOpenDatabase);
}

void RpcCreateVectorDiagram::onOpenDatabase()
{
    connect(m_dbLogger->getDb().get(), &VeinLogger::AbstractLoggerDB::sigDisplayActualValuesCompleted,
            this, &RpcCreateVectorDiagram::onDisplayActualValuesCompleted, Qt::QueuedConnection);
}

void RpcCreateVectorDiagram::callRPCFunction(const QUuid &callId, const QVariantMap &parameters)
{
    if(m_dbLogger->isDatabaseReady()) {
        m_options = parameters["p_paintingOptions"].toString();
        QString transaction = parameters["p_transaction"].toString();
        m_dbLogger->getDb()->startDisplayActualValues(callId, transaction);
    }
    else
        sendRpcError(callId, "Database is not set");
}

void RpcCreateVectorDiagram::onDisplayActualValuesCompleted(QUuid callId, bool success, QString errorMsg, QJsonObject loggedValues)
{
    if(!success)
        sendRpcError(callId, errorMsg);
    else
        sendRpcResult(callId, VectorDiagramCreator::CreateVectorDiagram(m_options, loggedValues));
}
