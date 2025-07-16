#ifndef RPCDISPLAYACTUALVALUES_H
#define RPCDISPLAYACTUALVALUES_H

#include <vf-cpp-rpc-simplified.h>

namespace VeinLogger {
class DatabaseLogger;
}

class RpcDisplayActualValues : public VfCpp::VfCppRpcSimplified
{
public:
    RpcDisplayActualValues(VeinLogger::DatabaseLogger *dbLogger,
                           int entityId);
private slots:
    void callRPCFunction(const QUuid &callId, const QVariantMap &parameters) override;
private:
    void RPC_displayActualValues(QUuid callId, QVariantMap parameters);
    VeinLogger::DatabaseLogger *m_dbLogger;
};

#endif // RPCDISPLAYACTUALVALUES_H
