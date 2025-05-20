#include "rpcdeletesession.h"
#include <vf-cpp-rpc-signature.h>

RpcDeleteSession::RpcDeleteSession(VeinEvent::EventSystem *eventSystem, int entityId, std::shared_ptr<VeinLogger::DatabaseCommandInterface> dbCmdInterface) :
    m_dbCmdInterface(dbCmdInterface),
    VfCpp::VfCppRpcSimplified(eventSystem,
                              entityId,
                              VfCpp::VfCppRpcSignature::createRpcSignature("RPC_deleteSession", VfCpp::VfCppRpcSignature::RPCParams({{"p_session", "QString"}})))
{
}

void RpcDeleteSession::callRPCFunction(const QUuid &callId, const QVariantMap &parameters)
{
    RPC_deleteSession(callId, parameters);
}

void RpcDeleteSession::RPC_deleteSession(QUuid callId, QVariantMap parameters)
{
    if(m_dbCmdInterface->isDatabaseConnected()) {
        QString session = parameters["p_session"].toString();
        emit m_dbCmdInterface->sigDeleteSession(callId, session);
    }
    else
        sendRpcError(callId, "Database is not set");
}
