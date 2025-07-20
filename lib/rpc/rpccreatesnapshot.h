#ifndef RPCCREATESNAPSHOT_H
#define RPCCREATESNAPSHOT_H

#include <vf-cpp-rpc-simplified.h>

namespace VeinLogger {
class DatabaseLogger;
}

class RpcCreateSnapshot : public VfCpp::VfCppRpcSimplified
{
    Q_OBJECT
public:
    RpcCreateSnapshot(VeinLogger::DatabaseLogger *dbLogger,
                      int entityId);
private slots:
    void callRPCFunction(const QUuid &callId, const QVariantMap &parameters) override;
    void onCreateSnapshotCompleted(QUuid callId, bool success, QString errorMsg);
private:
    VeinLogger::DatabaseLogger *m_dbLogger;
};

#endif // RPCCREATESNAPSHOT_H
