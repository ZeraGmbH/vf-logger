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
    void onOpenDatabase();
    void callRPCFunction(const QUuid &callId, const QVariantMap &parameters) override;
    void onListAllSessionsCompleted(QUuid callId, bool success, QString errorMsg, QJsonArray sessions);
private:
    void RPC_listAllSessions(QUuid callId);
    VeinLogger::DatabaseLogger *m_dbLogger;
};

#endif // RPCLISTALLSESSIONS_H
