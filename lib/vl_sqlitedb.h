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

    explicit SQLiteDB(QObject *t_parent = nullptr);
    ~SQLiteDB();

    bool requiresOwnThread() override { return true; }

    bool hasEntityId(int t_entityId) const override;
    bool hasComponentName(const QString &t_componentName) const override;
    bool hasSessionName(const QString &t_sessionName) const override;

    bool databaseIsOpen() const override;
    QString databasePath() const override;
    void setStorageMode(AbstractLoggerDB::STORAGE_MODE t_storageMode) override;
    AbstractLoggerDB::STORAGE_MODE getStorageMode() const override;

    static bool isValidDatabase(QString t_dbPath);

public slots:
    void initLocalData() override;
    void addComponent(const QString &t_componentName) override;
    void addEntity(int t_entityId, QString t_entityName) override;
    int addTransaction(const QString &t_transactionName, const QString &t_sessionName, const QString &t_contentSets, const QString &t_guiContextName) override;
    bool addStartTime(int t_transactionId, QDateTime t_time) override;
    bool addStopTime(int t_transactionId,  QDateTime t_time) override;
    bool deleteSession(const QString &t_session) override;
    int addSession(const QString &t_sessionName,QList<QVariantMap> p_staticData) override;
    void addLoggedValue(const  QString &t_sessionName, QVector<int> t_transactionIds, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp) override;
    QVariant readSessionComponent(const QString &p_session, const QString &p_enity, const QString &p_component) override;

    bool openDatabase(const QString &t_dbPath) override;
    bool isDbStillWitable(const QString &t_dbPath);

    void runBatchedExecution() override;

private:
    void addLoggedValue(int t_sessionId, QVector<int> transactionIds, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp);
    void writeStaticData(QVector<SQLBatchData> p_batchData);

private:
    DBPrivate *m_dPtr=nullptr;
};

} // namespace VeinLogger

#endif // VEINLOGGER_SQLITEDB_H
