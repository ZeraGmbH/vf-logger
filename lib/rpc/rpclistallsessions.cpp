#include "rpclistallsessions.h"
#include <vf-cpp-rpc-signature.h>

RpcListAllSessions::RpcListAllSessions(VeinEvent::EventSystem *eventSystem, int entityId, std::shared_ptr<VeinLogger::DatabaseCommandInterface> dbCmdInterface) :
    m_dbCmdInterface(dbCmdInterface),
    VfCpp::VfCppRpcSimplified(eventSystem,
                              entityId,
                              VfCpp::VfCppRpcSignature::createRpcSignature("RPC_listAllSessions", VfCpp::VfCppRpcSignature::RPCParams({})))
{
}

void RpcListAllSessions::callRPCFunction(const QUuid &callId, const QUuid &peerId, const QVariantMap &parameters)
{
    RPC_listAllSessions(callId);
}

void RpcListAllSessions::RPC_listAllSessions(QUuid callId)
{
    if(m_dbCmdInterface->isDatabaseConnected())
        emit m_dbCmdInterface->sigListAllSessions(callId);
    else
        sendRpcError(callId, "Database is not set");
}
