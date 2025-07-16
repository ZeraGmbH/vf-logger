#include "rpcdisplaysessionsinfos.h"
#include <vf-cpp-rpc-signature.h>

RpcDisplaySessionsInfos::RpcDisplaySessionsInfos(VeinEvent::EventSystem *eventSystem, int entityId, std::shared_ptr<VeinLogger::DatabaseCommandInterface> dbCmdInterface) :
    VfCpp::VfCppRpcSimplified(eventSystem,
                              entityId,
                              VfCpp::VfCppRpcSignature::createRpcSignature(
                                  "RPC_displaySessionsInfos",
                                  VfCpp::VfCppRpcSignature::RPCParams({{"p_session", "QString"}}))),
    m_dbCmdInterface(dbCmdInterface)
{
}

void RpcDisplaySessionsInfos::callRPCFunction(const QUuid &callId, const QVariantMap &parameters)
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
