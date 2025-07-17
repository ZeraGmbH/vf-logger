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
    void onDeleteSessionCompleted(QUuid callId, bool success, QString errorMsg);
private:
    VeinLogger::DatabaseLogger *m_dbLogger;
};

#endif // RPCDELETESESSION_H
