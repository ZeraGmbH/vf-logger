#ifndef RPCLISTALLSESSIONS_H
#define RPCLISTALLSESSIONS_H

#include "databasecommandinterface.h"
#include <vf-cpp-rpc-simplified.h>

class RpcListAllSessions : public VfCpp::VfCppRpcSimplified
{
public:
    RpcListAllSessions(VeinEvent::EventSystem *eventSystem, int entityId, std::shared_ptr<VeinLogger::DatabaseCommandInterface> dbCmdInterface);
private slots:
    void callRPCFunction(const QUuid &callId, const QVariantMap &parameters) override;
private:
    void RPC_listAllSessions(QUuid callId);
    std::shared_ptr<VeinLogger::DatabaseCommandInterface> m_dbCmdInterface = nullptr;
};

#endif // RPCLISTALLSESSIONS_H
