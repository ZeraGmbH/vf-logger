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
    void onOpenDatabase();
    void callRPCFunction(const QUuid &callId, const QVariantMap &parameters) override;
    void onDisplayActualValuesCompleted(QUuid callId, bool success, QString errorMsg, QJsonObject values);
private:
    VeinLogger::DatabaseLogger *m_dbLogger;
};

#endif // RPCDISPLAYACTUALVALUES_H
