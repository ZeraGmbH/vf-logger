#ifndef RPCDELETESESSION_H
#define RPCDELETESESSION_H

#include "databasecommandinterface.h"
#include <vf-cpp-rpc-simplified.h>

class RpcDeleteSession : public VfCpp::VfCppRpcSimplified
{
public:
    RpcDeleteSession(VeinEvent::EventSystem *eventSystem,
                     int entityId,
                     std::shared_ptr<VeinLogger::DatabaseCommandInterface> dbCmdInterface);
private slots:
    void callRPCFunction(const QUuid &callId, const QVariantMap &parameters) override;
private:
    void RPC_deleteSession(QUuid callId, QVariantMap parameters);
    std::shared_ptr<VeinLogger::DatabaseCommandInterface> m_dbCmdInterface = nullptr;
};

#endif // RPCDELETESESSION_H
