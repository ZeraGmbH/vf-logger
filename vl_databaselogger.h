#ifndef VL_DATALOGGER_H
#define VL_DATALOGGER_H


#include <ve_eventsystem.h>
#include <QDateTime>

#include <vfcpp.h>

#include "globalIncludes.h"
#include "vl_abstractloggerdb.h"
#include "jsoncontextloader.h"


#define stringify( name ) # name

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
class VFLOGGER_EXPORT DatabaseLogger : public VfCpp::VeinModuleEntity
{
    Q_OBJECT
// Vein Interface
public:
    // if you change something here make sure to change the component as well
    enum class LogType{
        snapshot,
        startStop,
        duration
    };
private:
    VfCpp::VeinSharedComp<QString> m_loggingStatus;
    VfCpp::VeinSharedComp<bool> m_loggingEnabled;
    VfCpp::VeinSharedComp<QList<int>> m_activeTransaction;
    VfCpp::VeinSharedComp<bool> m_databaseReady;
    VfCpp::VeinSharedComp<QString> m_databaseFile;
    VfCpp::VeinSharedComp<QString> m_databaseErrorFile;
    VfCpp::VeinSharedComp<QString> m_databaseFileMimeType;
    VfCpp::VeinSharedComp<long long> m_databaseFileSize;
    VfCpp::VeinSharedComp<QVariantMap> m_filesystemInfo;
    VfCpp::VeinSharedComp<QString> m_filesystemFree;
    VfCpp::VeinSharedComp<QString> m_filesystemTotal;
    VfCpp::VeinSharedComp<bool> m_scheduledLoggingEnabled;
    VfCpp::VeinSharedComp<int> m_scheduledLoggingCountdown;
    VfCpp::VeinSharedComp<QStringList> m_existingSessions;
    VfCpp::VeinSharedComp<QString> m_customerData;
    VfCpp::VeinSharedComp<QString> m_sessionName;
    VfCpp::VeinSharedComp<QStringList> m_availableContentSets;
    VfCpp::VeinSharedComp<QString> m_sessionProxy;
public slots:
    QVariant RPC_deleteSession(QVariantMap p_parameters);
    QVariant RPC_readTransaction(QVariantMap p_parameters);
    QVariant RPC_readSessionComponent(QVariantMap p_parameters);
    QVariant RPC_startLogging(QVariantMap p_parameters);
    QVariant RPC_stopLogging(QVariantMap p_parameters);
//Actual class
public:
    explicit DatabaseLogger(DataSource *t_dataSource, VeinLogger::DBFactory t_factoryFunction, QObject *t_parent=nullptr, AbstractLoggerDB::STORAGE_MODE t_storageMode=AbstractLoggerDB::STORAGE_MODE::TEXT);
    ~DatabaseLogger();
    /**
     * @brief removeScript
     * @param t_script
     *
     * Removes a script and stops a transaction
     */
    bool loggingEnabled() const;
    int entityId() const;
    QString entityName() const;
    bool setContentSetPath(const QString &p_zeraContentSetPath, const QString &p_customerContentSetPath);

    // Functions behind rpcs
    bool deleteSession(QString p_session);
    QJsonDocument readTransaction(QString p_session, QString p_transaction);
    QVariant readSessionComponent(QString p_session, QString p_entity, QString p_component);
    int startLogging(QString p_transactionName, DatabaseLogger::LogType p_type, int p_duration,QString p_guiContext, QStringList p_contentSets);
    bool stopLogging(QList<int> p_transactions);
public slots:
    void initOnce();

    /**
     * @brief addScript
     * @param t_script
     *
     * Adds a script and starts a transaction
     *
     */
    virtual void setLoggingEnabled(bool t_enabled);
    virtual void openDatabase(QVariant p_filePath);
    virtual void closeDatabase();
    virtual void checkDatabaseStillValid();


    /**
     * @brief updateSessionList
     * @param p_sessions: list of sessions stored in open database
     *
     * This function updates the Vein Component ExistingSessions to p_sessions
     */
    virtual void updateSessionList(QStringList p_sessions);
private slots:
    void readSession(QString session);
    void openSession(QVariant p_session);
    // EventSystem interface
public:
    virtual bool processEvent(QEvent *t_event) override;

    VfCpp::VeinSharedComp<bool> DatabaseReady() const;
    void setDatabaseReady(const VfCpp::VeinSharedComp<bool> &DatabaseReady);

private:
    void initEntity();

    QMap<int,QStringList> readContentSets(QStringList p_contentSets);


    DataLoggerPrivate *m_dPtr=nullptr;
    JsonContentSetLoader m_contentSetLoader;


    bool m_isInitilized;

    QMap<int,QMap<int, QStringList>> m_transactionList;
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
    void sigSingleShot();
    void sigLogSchedulerActivated();
    void sigLogSchedulerDeactivated();


    friend DataLoggerPrivate;
};
}

#endif // VL_DATALOGGER_H
