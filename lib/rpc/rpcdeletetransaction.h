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
    void callRPCFunction(const QUuid &callId, const QVariantMap &parameters) override;
    void onOpenDatabase();
    void onDeleteTransactionCompleted(QUuid callId, bool success, QString errorMsg);
private:
    void RPC_deleteTransaction(QUuid callId, QVariantMap parameters);
    VeinLogger::DatabaseLogger *m_dbLogger;
};

#endif // RPCDELETETRANSACTION_H
