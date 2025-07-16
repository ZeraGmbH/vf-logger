#ifndef RPCDISPLAYACTUALVALUES_H
#define RPCDISPLAYACTUALVALUES_H

#include "databasecommandinterface.h"
#include <vf-cpp-rpc-simplified.h>

class RpcDisplayActualValues : public VfCpp::VfCppRpcSimplified
{
public:
    RpcDisplayActualValues(VeinEvent::EventSystem *eventSystem,
                           int entityId,
                           std::shared_ptr<VeinLogger::DatabaseCommandInterface> dbCmdInterface);
private slots:
    void callRPCFunction(const QUuid &callId, const QVariantMap &parameters) override;
private:
    void RPC_displayActualValues(QUuid callId, QVariantMap parameters);
    std::shared_ptr<VeinLogger::DatabaseCommandInterface> m_dbCmdInterface = nullptr;
};

#endif // RPCDISPLAYACTUALVALUES_H
