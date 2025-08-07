#ifndef RPCDISPLAYCUSTOMERDATA_H
#define RPCDISPLAYCUSTOMERDATA_H

#include <vf-cpp-rpc-simplified.h>
#include <QUuid>

namespace VeinLogger {
class DatabaseLogger;
}

class RpcDisplayCustomerData : public VfCpp::VfCppRpcSimplified
{
public:
    RpcDisplayCustomerData(VeinLogger::DatabaseLogger *dbLogger, int entityId);
private slots:
    void onOpenDatabase();
    void callRPCFunction(const QUuid &callId, const QVariantMap &parameters) override;
    void onDisplayCustomerDataCompleted(QUuid callId, bool success, QString errorMsg, QJsonObject custData);
private:
    VeinLogger::DatabaseLogger *m_dbLogger;
};

#endif // RPCDISPLAYCUSTOMERDATA_H
