#ifndef RPCDELETESESSION_H
#define RPCDELETESESSION_H

#include <vf-cpp-rpc-simplified.h>

namespace VeinLogger {
class DatabaseLogger;
}

class RpcDeleteSession : public VfCpp::VfCppRpcSimplified
{
public:
    RpcDeleteSession(VeinLogger::DatabaseLogger *dbLogger,
                     int entityId);
private slots:
    void callRPCFunction(const QUuid &callId, const QVariantMap &parameters) override;
private:
    void RPC_deleteSession(QUuid callId, QVariantMap parameters);
    VeinLogger::DatabaseLogger *m_dbLogger;
};

#endif // RPCDELETESESSION_H
