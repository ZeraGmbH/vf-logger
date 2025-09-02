#ifndef RPCCREATEVECTORDIAGRAM_H
#define RPCCREATEVECTORDIAGRAM_H

#include <vf-cpp-rpc-simplified.h>
#include <QJsonObject>

namespace VeinLogger {
class DatabaseLogger;
}

class RpcCreateVectorDiagram : public VfCpp::VfCppRpcSimplified
{
    Q_OBJECT
public:
    RpcCreateVectorDiagram(VeinLogger::DatabaseLogger *dbLogger,
                           int entityId);
private slots:
    void onOpenDatabase();
    void callRPCFunction(const QUuid &callId, const QVariantMap &parameters) override;
    void onDisplayActualValuesCompleted(QUuid callId, bool success, QString errorMsg, QJsonObject loggedValues);
private:
    VeinLogger::DatabaseLogger *m_dbLogger;
    QString m_options;
};

#endif // RPCCREATEVECTORDIAGRAM_H
