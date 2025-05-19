#include "rpcdisplayactualvalues.h"
#include <vf-cpp-rpc-signature.h>

RpcDisplayActualValues::RpcDisplayActualValues(VeinEvent::EventSystem *eventSystem, int entityId, std::shared_ptr<VeinLogger::DatabaseCommandInterface> dbCmdInterface) :
    m_dbCmdInterface(dbCmdInterface),
    VfCpp::VfCppRpcSimplified(eventSystem,
                              entityId,
                              VfCpp::VfCppRpcSignature::createRpcSignature("RPC_displayActualValues", VfCpp::VfCppRpcSignature::RPCParams({{"p_transaction", "QString"}})))
{
}

void RpcDisplayActualValues::callRPCFunction(const QUuid &callId, const QUuid &peerId, const QVariantMap &parameters)
{
    RPC_displayActualValues(callId, parameters);
}

void RpcDisplayActualValues::RPC_displayActualValues(QUuid callId, QVariantMap parameters)
{
    if(m_dbCmdInterface->isDatabaseConnected()) {
        QString transaction = parameters["p_transaction"].toString();
        emit m_dbCmdInterface->sigDisplayActualValues(callId, transaction);
    }
    else
        sendRpcError(callId, "Database is not set");
}
