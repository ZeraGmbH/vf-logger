#ifndef RPCLISTALLSESSIONS_H
#define RPCLISTALLSESSIONS_H

#include <vf-cpp-rpc-simplified.h>

namespace VeinLogger {
class DatabaseLogger;
}

class RpcListAllSessions : public VfCpp::VfCppRpcSimplified
{
public:
    RpcListAllSessions(VeinLogger::DatabaseLogger *dbLogger,
                       int entityId);
private slots:
    void callRPCFunction(const QUuid &callId, const QVariantMap &parameters) override;
private:
    void RPC_listAllSessions(QUuid callId);
    VeinLogger::DatabaseLogger *m_dbLogger;
};

#endif // RPCLISTALLSESSIONS_H
