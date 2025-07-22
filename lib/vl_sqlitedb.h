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
    int addTransaction(const QString &transactionName, const QString &sessionName, const QStringList &contentSets, const QString &guiContextName) override;
    bool updateTransactionStartTime(int transactionId, QDateTime time) override;
    int addSession(const QString &sessionName, QList<VeinLogger::ComponentInfo> componentsStoredOncePerSession) override;
    void addLoggedValue(const  QString &sessionName, QVector<int> transactionIds, VeinLogger::ComponentInfo component) override;

    void onOpen(const QString &dbPath) override;
    bool isDbStillWitable(const QString &dbPath);

protected:
    bool updateTransactionStopTime(int transactionId,  QDateTime time) override;
    virtual void addComponent(const QString &componentName);
    virtual void addEntity(int entityId, QString entityName);

private:
    void onDeleteTransaction(QUuid callId, const QString &transactionName) override;
    void onDisplaySessionsInfos(QUuid callId, const QString &sessionName) override;
    void onListAllSessions(QUuid callId) override;
    void onDisplayActualValues(QUuid callId, const QString &transactionName) override;
    void onDeleteSession(QUuid callId, const QString &session) override;
    void onFlushToDb() override;

    void initLocalData();
    bool hasEntityId(int entityId) const;
    bool hasComponentName(const QString &componentName) const;
    void addEntityComponent(const ComponentInfo &component);
    void addLoggedValue(int sessionId, const QVector<int> &transactionIds, const ComponentInfo &component);
    void writeStaticData(QVector<SQLBatchData> p_batchData);
    QStringList getContentsetList(const QString &transactionName);

    DBPrivate *m_dPtr = nullptr;
};

} // namespace VeinLogger

#endif // VEINLOGGER_SQLITEDB_H
