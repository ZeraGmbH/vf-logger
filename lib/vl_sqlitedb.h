#ifndef VEINLOGGER_SQLITEDB_H
#define VEINLOGGER_SQLITEDB_H

#include "vflogger_export.h"
#include "vl_abstractloggerdb.h"
#include <QVector>
#include <QDateTime>
#include <QVariant>
#include <functional>

namespace VeinLogger
{
struct SQLBatchData
{
    int entityId;
    int componentId;
    QVector<int> transactionIds;
    int sessionId;
    QDateTime timestamp;
    QVariant value;
};

class DBPrivate;

class VFLOGGER_EXPORT SQLiteDB : public AbstractLoggerDB
{
    Q_OBJECT
public:
    explicit SQLiteDB();
    ~SQLiteDB();

    bool requiresOwnThread() override { return true; }

    bool hasSessionName(const QString &sessionName) const override;

    void setStorageMode(AbstractLoggerDB::STORAGE_MODE storageMode) override;

    static bool isValidDatabase(QString dbPath);

public slots:
    void initLocalData() override;
    int addTransaction(const QString &transactionName, const QString &sessionName, const QStringList &contentSets, const QString &guiContextName) override;
    bool addStartTime(int transactionId, QDateTime time) override;
    bool addStopTime(int transactionId,  QDateTime time) override;
    bool deleteSession(const QString &session) override;
    int addSession(const QString &sessionName, QList<VeinLogger::DatabaseCommandInterface::ComponentInfo> componentsStoredOncePerSession) override;
    void addLoggedValue(const  QString &sessionName, QVector<int> transactionIds, VeinLogger::DatabaseCommandInterface::ComponentInfo component) override;
    QVariant readSessionComponent(const QString &session, const QString &enity, const QString &component) override;
    QJsonObject displaySessionsInfos(const QString &sessionName) override;
    bool deleteTransaction(const QString &transactionName) override;
    QJsonArray displayAllSessions() override;
    QVariant displayValues(const QString &transactionName) override;

    void onOpen(const QString &dbPath) override;
    bool isDbStillWitable(const QString &dbPath);

    void runBatchedExecution() override;

protected:
    virtual void addComponent(const QString &componentName);
    virtual void addEntity(int entityId, QString entityName);

private:
    bool hasEntityId(int entityId) const;
    bool hasComponentName(const QString &componentName) const;
    void addEntityComponent(const DatabaseCommandInterface::ComponentInfo &component);
    void addLoggedValue(int sessionId, const QVector<int> &transactionIds, const DatabaseCommandInterface::ComponentInfo &component);
    void writeStaticData(QVector<SQLBatchData> p_batchData);
    void setValidTransactions();
    QStringList getContentsetList(const QString &transactionName);


    DBPrivate *m_dPtr = nullptr;
};

} // namespace VeinLogger

#endif // VEINLOGGER_SQLITEDB_H
