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
    void onOpenDatabase();
    void callRPCFunction(const QUuid &callId, const QVariantMap &parameters) override;
    void onDisplaySessionInfosCompleted(QUuid callId, bool success, QString errorMsg, QJsonObject infos);
private:
    VeinLogger::DatabaseLogger *m_dbLogger;
};

#endif // RPCDISPLAYSESSIONSINFOS_H
