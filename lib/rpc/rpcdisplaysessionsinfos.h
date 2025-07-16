#ifndef RPCDISPLAYSESSIONSINFOS_H
#define RPCDISPLAYSESSIONSINFOS_H

#include "databasecommandinterface.h"
#include <vf-cpp-rpc-simplified.h>

class RpcDisplaySessionsInfos : public VfCpp::VfCppRpcSimplified
{
public:
    RpcDisplaySessionsInfos(VeinEvent::EventSystem *eventSystem,
                            int entityId,
                            std::shared_ptr<VeinLogger::DatabaseCommandInterface> dbCmdInterface);
private slots:
    void callRPCFunction(const QUuid &callId, const QVariantMap &parameters) override;
private:
    void RPC_displaySessionsInfos(QUuid callId, QVariantMap parameters);
    std::shared_ptr<VeinLogger::DatabaseCommandInterface> m_dbCmdInterface = nullptr;
};

#endif // RPCDISPLAYSESSIONSINFOS_H
