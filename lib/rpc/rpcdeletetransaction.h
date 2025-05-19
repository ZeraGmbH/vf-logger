#ifndef RPCDELETETRANSACTION_H
#define RPCDELETETRANSACTION_H

#include "databasecommandinterface.h"
#include <vf-cpp-rpc-simplified.h>

class RpcDeleteTransaction : public VfCpp::VfCppRpcSimplified
{
public:
    RpcDeleteTransaction(VeinEvent::EventSystem *eventSystem, int entityId, std::shared_ptr<VeinLogger::DatabaseCommandInterface> dbCmdInterface);
private slots:
    void callRPCFunction(const QUuid &callId, const QUuid &peerId, const QVariantMap &parameters) override;
private:
    void RPC_deleteTransaction(QUuid callId, QVariantMap parameters);
    std::shared_ptr<VeinLogger::DatabaseCommandInterface> m_dbCmdInterface = nullptr;
};

#endif // RPCDELETETRANSACTION_H
