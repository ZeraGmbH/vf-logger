#ifndef TEST_RPC_SNAPSHOT_H
#define TEST_RPC_SNAPSHOT_H

#include "testloggersystem.h"
#include <QObject>

class test_rpc_snapshot : public QObject
{
    Q_OBJECT
private slots:
    void init();
    void cleanup();

    void invalidParams();
    void noDatabaseLoaded();
    void noSessionSet();
    void noTransactionSet();
    void emptyContentSet();

    void wip();
private:
    static QVariantMap createRpcParams(const QString &transactionName,
                                       const QString &sessionName,
                                       const QStringList &contentSets,
                                       const QString &guiContextName);
    static QVariantMap getValidRpcParams();
    QVariant getResultCode(QVariant rpcReturnData);
    QString getErrorString(QVariant rpcReturnData);

    std::unique_ptr<TestLoggerSystem> m_testSystem;
};

#endif // TEST_RPC_SNAPSHOT_H
