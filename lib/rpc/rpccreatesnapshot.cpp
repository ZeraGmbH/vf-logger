#include "rpccreatesnapshot.h"
#include "vl_databaselogger.h"
#include <vf-cpp-rpc-signature.h>

RpcCreateSnapshot::RpcCreateSnapshot(VeinLogger::DatabaseLogger *dbLogger,
                                     int entityId) :
    VfCpp::VfCppRpcSimplified(dbLogger,
                              entityId,
                              VfCpp::VfCppRpcSignature::createRpcSignature(
                                    "RPC_createSnapshot",
                                    VfCpp::VfCppRpcSignature::RPCParams({{"p_transactionName", "QString"},
                                                                         {"p_sessionName", "QString"},
                                                                         {"p_contentSets", "QStringList"},
                                                                         {"p_guiContext", "QString"}}))),
    m_dbLogger(dbLogger)
{
    connect(m_dbLogger, &VeinLogger::DatabaseLogger::sigTakeSnapshotCompleted,
            this, &RpcCreateSnapshot::onCreateSnapshotCompleted);
}

void RpcCreateSnapshot::callRPCFunction(const QUuid &callId, const QVariantMap &parameters)
{
    const QString transactionName = parameters["p_transactionName"].toString();
    const QString sessionName = parameters["p_sessionName"].toString();
    const QStringList contentSets = parameters["p_contentSets"].toStringList();
    const QString guiContext = parameters["p_guiContext"].toString();

    QStringList errs;
    if (!m_dbLogger->isDatabaseReady())
        errs.append("Logging requires a database!");
    if (transactionName.isEmpty())
        errs.append("Cannot create snapshot with empty transaction name!");
    if (sessionName.isEmpty())
        errs.append("Cannot create snapshot with empty session name!");
    if (contentSets.isEmpty())
        errs.append("Cannot create snapshot with empty content sets");

    if(errs.isEmpty()) {
        VeinLogger::StartTransactionParam param = {transactionName, sessionName, contentSets, guiContext};
        m_dbLogger->startTakeSnapshot(callId, param);
    }
    else
        sendRpcError(callId, QString("Conditions to create snapshot are not met:\n%1").arg(errs.join("\n")));
}

void RpcCreateSnapshot::onCreateSnapshotCompleted(QUuid callId, bool success, QString errorMsg)
{

}
