#include "rpcdisplaysessionsinfos.h"
#include <vf-cpp-rpc-signature.h>

RpcDisplaySessionsInfos::RpcDisplaySessionsInfos(VeinEvent::EventSystem *eventSystem, int entityId, std::shared_ptr<VeinLogger::DatabaseCommandInterface> dbCmdInterface) :
    m_dbCmdInterface(dbCmdInterface),
    VfCpp::VfCppRpcSimplified(eventSystem,
                              entityId,
                              VfCpp::VfCppRpcSignature::createRpcSignature("RPC_displaySessionsInfos", VfCpp::VfCppRpcSignature::RPCParams({{"p_session", "QString"}})))
{
}

void RpcDisplaySessionsInfos::callRPCFunction(const QUuid &callId, const QUuid &peerId, const QVariantMap &parameters)
{
    RPC_displaySessionsInfos(callId, parameters);
}

void RpcDisplaySessionsInfos::RPC_displaySessionsInfos(QUuid callId, QVariantMap parameters)
{
    if(m_dbCmdInterface->isDatabaseConnected()) {
        QString session = parameters["p_session"].toString();
        emit m_dbCmdInterface->sigDisplaySessionInfos(callId, session);
    }
    else
        sendRpcError(callId, "Database is not set");
}
