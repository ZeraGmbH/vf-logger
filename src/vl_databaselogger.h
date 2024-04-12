#ifndef VL_DATALOGGER_H
#define VL_DATALOGGER_H

#include "vflogger_export.h"
#include "vl_abstractloggerdb.h"
#include <ve_eventsystem.h>
#include <QDateTime>


namespace VeinLogger
{
class DataLoggerPrivate;
class DataSource;
class QmlLogger;

/**
 * @brief The DatabaseLogger class
 * Interface to vein logger
 *
 * @todo add rpc to start recording
 * @todo add rpc to stop recording
 * @todo add rpc to add contentSet
 * @todo add rpc to remove contentSet
 * @todo add rpc to create snapshot
 */
class VFLOGGER_EXPORT DatabaseLogger : public VeinEvent::EventSystem
{
    Q_OBJECT

public:
    explicit DatabaseLogger(DataSource *t_dataSource, VeinLogger::DBFactory t_factoryFunction, QObject *t_parent=nullptr, AbstractLoggerDB::STORAGE_MODE t_storageMode=AbstractLoggerDB::STORAGE_MODE::TEXT);
    ~DatabaseLogger();
    virtual void processEvent(QEvent *t_event) override;
    /**
     * @brief addScript
     * @param t_script
     *
     * Adds a script and starts a transaction
     *
     */
    virtual void addScript(QmlLogger *t_script);
    /**
     * @brief removeScript
     * @param t_script
     *
     * Removes a script and stops a transaction
     */
    virtual void removeScript(QmlLogger *t_script);
    bool loggingEnabled() const;
    int entityId() const;
    QString entityName() const;

signals:
    /**
     * @brief sigAddLoggedValue
     * @param t_sessionName: used session Name
     * @param t_transactionIds: sql id of transaction
     * @param t_entityId: sql entity id of value
     * @param t_componentName: sql component id of value
     * @param t_value: value: itself
     * @param t_timestamp: time the value change occured
     */
    void sigAddLoggedValue(QString t_sessionName, QVector<int> t_transactionIds, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp);
    void sigAddEntity(int t_entityId, const QString &t_entityName);
    void sigAddComponent(const QString &t_componentName);
    void sigAddSession(const QString &t_sessionName,QList<QVariantMap> p_staticData);

    void sigOpenDatabase(const QString &t_filePath);

    void sigDatabaseError(const QString &t_errorString);
    void sigDatabaseReady();
    void sigDatabaseUnloaded();
    void sigLoggingEnabledChanged(bool t_enabled);
    void sigLoggingStarted();
    void sigLoggingStopped();
    void sigLogSchedulerActivated();
    void sigLogSchedulerDeactivated();
public slots:
    virtual void setLoggingEnabled(bool t_enabled);
    virtual bool openDatabase(const QString &t_filePath);
    virtual void closeDatabase();
    virtual void checkDatabaseStillValid();
    QVariant RPC_deleteSession(QVariantMap p_parameters);
    QVariant RPC_readTransaction(QVariantMap p_parameters);
    QVariant RPC_readSessionComponent(QVariantMap p_parameters);
    /**
     * @brief updateSessionList
     * @param p_sessions: list of sessions stored in open database
     *
     * This function updates the Vein Component ExistingSessions to p_sessions
     */
    virtual void updateSessionList(QStringList p_sessions);

private:
    void initEntity();

    DataLoggerPrivate *m_dPtr=nullptr;
};
}

#endif // VL_DATALOGGER_H
