#ifndef VEINLOGGER_SQLITEDB_H
#define VEINLOGGER_SQLITEDB_H

#include "globalIncludes.h"
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

    bool hasEntityId(int t_entityId) const override;
    bool hasComponentName(const QString &t_componentName) const override;
    bool hasSessionName(const QString &t_sessionName) const override;

    bool databaseIsOpen() const override;
    QString databasePath() const override;
    void setStorageMode(AbstractLoggerDB::STORAGE_MODE t_storageMode) override;
    AbstractLoggerDB::STORAGE_MODE getStorageMode() const override;
    std::function<bool(QString)> getDatabaseValidationFunction() const override;

    QJsonDocument  readTransaction(const QString &p_transaction, const QString &p_session);

    static bool isValidDatabase(QString t_dbPath);

public slots:
    void initLocalData() override;
    void addComponent(const QString &t_componentName) override;
    void addEntity(int t_entityId, QString t_entityName) override;
    int addTransaction(const QString &t_transactionName, const QString &t_sessionName, const QString &t_contentSets, const QString &t_guiContextName) override;
    bool addStartTime(int t_transactionId, QDateTime t_time) override;
    bool addStopTime(int t_transactionId,  QDateTime t_time) override;
    int addSession(const QString &t_sessionName) override;
    void addLoggedValue(int t_sessionId, QVector<int> transactionIds, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp) override;
    void addLoggedValue(const  QString &t_sessionName, QVector<int> t_transactionIds, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp) override;

    bool openDatabase(const QString &t_dbPath) override;


    void runBatchedExecution() override;

private:
    DBPrivate *m_dPtr=nullptr;
};

} // namespace VeinLogger

#endif // VEINLOGGER_SQLITEDB_H
