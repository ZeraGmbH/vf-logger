#ifndef RPCDELETETRANSACTION_H
#define RPCDELETETRANSACTION_H

#include <vf-cpp-rpc-simplified.h>

namespace VeinLogger {
class DatabaseLogger;
}

class RpcDeleteTransaction : public VfCpp::VfCppRpcSimplified
{
public:
    RpcDeleteTransaction(VeinLogger::DatabaseLogger *dbLogger, int entityId);
private slots:
    void onOpenDatabase();
    void callRPCFunction(const QUuid &callId, const QVariantMap &parameters) override;
    void onDeleteTransactionCompleted(QUuid callId, bool success, QString errorMsg);
private:
    VeinLogger::DatabaseLogger *m_dbLogger;
};

#endif // RPCDELETETRANSACTION_H
