#ifndef RPCDISPLAYSESSIONSINFOS_H
#define RPCDISPLAYSESSIONSINFOS_H

#include <vf-cpp-rpc-simplified.h>
#include <QUuid>

namespace VeinLogger {
class DatabaseLogger;
}

class RpcDisplaySessionsInfos : public VfCpp::VfCppRpcSimplified
{
public:
    RpcDisplaySessionsInfos(VeinLogger::DatabaseLogger *dbLogger, int entityId);
private slots:
    void callRPCFunction(const QUuid &callId, const QVariantMap &parameters) override;
private:
    void RPC_displaySessionsInfos(QUuid callId, QVariantMap parameters);
    VeinLogger::DatabaseLogger *m_dbLogger;
};

#endif // RPCDISPLAYSESSIONSINFOS_H
