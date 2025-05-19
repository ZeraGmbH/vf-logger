#include "rpcdeletetransaction.h"
#include <vf-cpp-rpc-signature.h>


RpcDeleteTransaction::RpcDeleteTransaction(VeinEvent::EventSystem *eventSystem, int entityId, std::shared_ptr<VeinLogger::DatabaseCommandInterface> dbCmdInterface) :
    m_dbCmdInterface(dbCmdInterface),
    VfCpp::VfCppRpcSimplified(eventSystem,
                              entityId,
                              VfCpp::VfCppRpcSignature::createRpcSignature("RPC_deleteTransaction", VfCpp::VfCppRpcSignature::RPCParams({{"p_transaction", "QString"}})))
{
}

void RpcDeleteTransaction::callRPCFunction(const QUuid &callId, const QUuid &peerId, const QVariantMap &parameters)
{
    RPC_deleteTransaction(callId, parameters);
}

void RpcDeleteTransaction::RPC_deleteTransaction(QUuid callId, QVariantMap parameters)
{
    if(m_dbCmdInterface->isDatabaseConnected()) {
        QString transaction = parameters["p_transaction"].toString();
        emit m_dbCmdInterface->sigDeleteTransaction(callId, transaction);
    }
    else
        sendRpcError(callId, "Database is not set");
}
