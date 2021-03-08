#ifndef VL_DATABASELOGGERPRIVATE_H
#define VL_DATABASELOGGERPRIVATE_H

#include <QThread>
#include <QTimer>
#include <QStateMachine>
#include <QStorageInfo>
#include <QFileSystemWatcher>
#include <QMimeDatabase>

#include <vcmp_componentdata.h>
#include <vcmp_entitydata.h>
#include <vcmp_errordata.h>
#include <ve_commandevent.h>
#include <veinmodulerpc.h>

#include "vl_databaselogger.h"


namespace VeinLogger {


class DataLoggerPrivate: public QObject
{
//functions
public:
    explicit DataLoggerPrivate(DatabaseLogger *t_qPtr);
    ~DataLoggerPrivate();

    void initOnce();

    void setStatusText(const QString &t_status);

    void initStateMachine();

    void updateDBStorageInfo();

    bool checkDBFilePath(const QString &t_dbFilePath);

    void updateDBFileSizeInfo();

    void updateSchedulerCountdown();
//functions
private:
//Variables
private:

    /**
     * @brief The logging is implemented via interpreted scripts that state which values to log
     * @see vl_qmllogger.cpp
     */
    QVector<QmlLogger *> m_loggerScripts;

    /**
     * @brief The actual database choice is an implementation detail of the DatabaseLogger
     */
    AbstractLoggerDB *m_database=nullptr;
    DBFactory m_databaseFactory;
    QString m_databaseFilePath;
    DataSource *m_dataSource=nullptr;

    /**
     * @brief Qt doesn't support non blocking database access
     */
    QThread m_asyncDatabaseThread;
    /**
     * @b Logging in batches is much more efficient for SQLITE (and for spinning disk storages in general)
     * @note The batch timer is independent from the recording timeframe as it only pushes already logged values to the database
     */
    QTimer m_batchedExecutionTimer;
    /**
     * @brief logging duration in ms
     */
    int m_scheduledLoggingDuration;

    QFileSystemWatcher m_deleteWatcher;
    bool m_noUninitMessage = false;

    QTimer m_schedulingTimer;
    QTimer m_countdownUpdateTimer;
    bool m_initDone=false;
    QString m_loggerStatusText="Logging inactive";

    QMap<QString,VfCpp::cVeinModuleRpc::Ptr> m_rpcList;
    /**
     * @brief m_sessionName
     * stores the current session Name.
     *
     * We need this, when deleting a session, to compare values.
     * That's a quite ugly solution. However using vf-cpp in future those things
     * will work out better.
     */
    QString m_sessionName;

    int m_entityId;
    //entity name
    QLatin1String m_entityName;
    //component names
    static constexpr QLatin1String s_entityNameComponentName = QLatin1String("EntityName");
    static constexpr QLatin1String s_loggingStatusTextComponentName = QLatin1String("LoggingStatus");
    static constexpr QLatin1String s_loggingEnabledComponentName = QLatin1String("LoggingEnabled");
    static constexpr QLatin1String s_databaseReadyComponentName = QLatin1String("DatabaseReady");
    static constexpr QLatin1String s_databaseFileComponentName = QLatin1String("DatabaseFile");
    static constexpr QLatin1String s_databaseErrorFileComponentName = QLatin1String("DatabaseErrorFile");
    static constexpr QLatin1String s_databaseFileMimeTypeComponentName = QLatin1String("DatabaseFileMimeType");
    static constexpr QLatin1String s_databaseFileSizeComponentName = QLatin1String("DatabaseFileSize");
    static constexpr QLatin1String s_filesystemInfoComponentName = QLatin1String("FilesystemInfo");
    static constexpr QLatin1String s_filesystemFreePropertyName = QLatin1String("FilesystemFree");
    static constexpr QLatin1String s_filesystemTotalPropertyName = QLatin1String("FilesystemTotal");
    static constexpr QLatin1String s_scheduledLoggingEnabledComponentName = QLatin1String("ScheduledLoggingEnabled");
    static constexpr QLatin1String s_scheduledLoggingDurationComponentName = QLatin1String("ScheduledLoggingDuration");
    static constexpr QLatin1String s_scheduledLoggingCountdownComponentName = QLatin1String("ScheduledLoggingCountdown");
    static constexpr QLatin1String s_existingSessionsComponentName = QLatin1String("ExistingSessions");
    static constexpr QLatin1String s_customerDataComponentName = QLatin1String("CustomerData");

    // TODO: Add more from modulemanager
    static constexpr QLatin1String s_sessionNameComponentName = QLatin1String("sessionName");
    static constexpr QLatin1String s_guiContextComponentName = QLatin1String("guiContext");
    static constexpr QLatin1String s_transactionNameComponentName = QLatin1String("transactionName");
    static constexpr QLatin1String s_currentContentSetsComponentName = QLatin1String("currentContentSets");
    static constexpr QLatin1String s_availableContentSetsComponentName = QLatin1String("availableContentSets");

    QStateMachine m_stateMachine;
    //QStatemachine does not support ParallelState
    //Therefore we add one state where the actual statemachine starts
    QState *m_parallelWrapperState = new QState(&m_stateMachine);

    QState *m_databaseContainerState = new QState(m_parallelWrapperState);
    QState *m_databaseUninitializedState = new QState(m_databaseContainerState);
    QState *m_databaseReadyState = new QState(m_databaseContainerState);

    QState *m_loggingContainerState = new QState(m_parallelWrapperState);
    QState *m_loggingEnabledState = new QState(m_loggingContainerState);
    QState *m_loggingDisabledState = new QState(m_loggingContainerState);

    QState *m_logSchedulerContainerState = new QState(m_parallelWrapperState);
    QState *m_logSchedulerEnabledState = new QState(m_logSchedulerContainerState);
    QState *m_logSchedulerDisabledState = new QState(m_logSchedulerContainerState);

    AbstractLoggerDB::STORAGE_MODE m_storageMode;

    DatabaseLogger *m_qPtr=nullptr;
    friend class DatabaseLogger;
};


}

#endif // VL_DATABASELOGGERPRIVATE_H
