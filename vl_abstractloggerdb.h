#ifndef VEINLOGGER_ABSTRACTLOGGERDB_H
#define VEINLOGGER_ABSTRACTLOGGERDB_H

#include <QObject>
#include <QVector>
#include <QDateTime>
#include <QVariant>
#include <functional>

namespace VeinLogger
{
class AbstractLoggerDB : public QObject
{
    Q_OBJECT

public:
    AbstractLoggerDB(QObject *t_parent=nullptr);
    virtual ~AbstractLoggerDB()
    {

    }

    enum class STORAGE_MODE : int {
        TEXT = 0,
        BINARY = 1,
    };

    virtual bool hasEntityId(int t_entityId) const =0;
    virtual bool hasComponentName(const QString &t_componentName) const =0;
    virtual bool hasSessionName(const QString &t_sessionName) const =0;

    virtual bool databaseIsOpen() const =0;
    virtual QString databasePath() const =0;
    virtual void setStorageMode(STORAGE_MODE t_storageMode) =0;
    virtual STORAGE_MODE getStorageMode() const =0;
    virtual std::function<bool(QString)> getDatabaseValidationFunction() const =0;

signals:
    void sigDatabaseError(const QString &t_errorString);
    void sigDatabaseReady();
    void sigNewSessionList(QStringList p_sessions);

public slots:
    virtual void initLocalData() =0;
    virtual void addComponent(const QString &t_componentName) =0;
    virtual void addEntity(int t_entityId, QString t_entityName) =0;
    virtual int addTransaction(const QString &t_transactionName, const QString &t_sessionName, const QString &t_contentSets, const QString &t_guiContextName) =0;
    /**
     * @brief addStartTime
     * @param t_transactionId: sql transaction id
     * @param t_time: transaction start time
     * @return true
     *
     * set the snapshot time or recording start time
     */
    virtual bool addStartTime(int t_transactionId, QDateTime t_time) = 0;
    /**
     * @brief addStopTime
     * @param t_transactionId: sql transaction id
     * @param t_time: transaction stop time
     * @return true
     *
     * set the snapshot time or recording stop time.
     */
    virtual bool addStopTime(int t_transactionId,  QDateTime t_time) = 0;

    virtual int addSession(const QString &t_sessionName,QList<QVariantMap> p_staticData) =0;
    /**
     * @brief addLoggedValue
     * @param t_sessionId
     * @param t_transactionIds
     * @param t_entityId
     * @param t_componentName
     * @param t_value
     * @param t_timestamp
     *
     * @todo Remove sessionId. Its not necessary and forces the user to store only in one session at the same time.
     */
    virtual void addLoggedValue(int t_sessionId, QVector<int> t_transactionIds, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp) =0;
    /**
     * @brief addLoggedValue
     * @param t_sessionName
     * @param t_transactionIds
     * @param t_entityId
     * @param t_componentName
     * @param t_value
     * @param t_timestamp
     *
     * @todo Remove sessionName. Its not necessary and forces the user to store only in one session at the same time.
     */
    virtual void addLoggedValue(const QString &t_sessionName, QVector<int> t_transactionIds, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp) =0;

    virtual bool openDatabase(const QString &t_dbPath) =0;
    virtual void runBatchedExecution() =0;
};

/// @b factory function alias to create database
using DBFactory = std::function<AbstractLoggerDB *()>;
} // namespace VeinLogger

#endif // VEINLOGGER_ABSTRACTLOGGERDB_H
