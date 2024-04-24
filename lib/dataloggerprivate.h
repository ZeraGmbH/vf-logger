#ifndef DATALOGGERPRIVATE_H
#define DATALOGGERPRIVATE_H

#include "vl_databaselogger.h"
#include <vf-cpp-rpc.h>
#include <QVector>
#include <QTimer>
#include <QFileSystemWatcher>
#include <QThread>
#include <QStateMachine>

class DataLoggerPrivate: public QObject
{
public:
    explicit DataLoggerPrivate(VeinLogger::DatabaseLogger *qPtr);
    ~DataLoggerPrivate();

    void initOnce();
    void setStatusText(const QString &status);
    void initStateMachine();
    void updateSchedulerCountdown();

    QString m_databaseFilePath;

    QThread m_asyncDatabaseThread;
    /**
     * @b Logging in batches is much more efficient for SQLITE (and for spinning disk storages in general)
     * @note The batch timer is independent from the recording timeframe as it only pushes already logged values to the database
     */
    QTimer m_batchedExecutionTimer;
    int m_scheduledLoggingDurationMs;

    QFileSystemWatcher m_deleteWatcher;
    bool m_noUninitMessage = false;

    QTimer m_schedulingTimer;
    QTimer m_countdownUpdateTimer;
    bool m_initDone=false;
    QString m_loggerStatusText="Logging inactive";

    QMap<QString, VfCpp::cVeinModuleRpc::Ptr> m_rpcList;

    //entity name
    QLatin1String m_entityName;
    //component names
    static const QLatin1String s_entityNameComponentName;
    static const QLatin1String s_loggingStatusTextComponentName;
    static const QLatin1String s_loggingEnabledComponentName;
    static const QLatin1String s_databaseReadyComponentName;
    static const QLatin1String s_databaseFileComponentName;
    static const QLatin1String s_scheduledLoggingEnabledComponentName;
    static const QLatin1String s_scheduledLoggingDurationComponentName;
    static const QLatin1String s_scheduledLoggingCountdownComponentName;
    static const QLatin1String s_existingSessionsComponentName;

    static const QLatin1String s_customerDataComponentName;
    static const QLatin1String s_sessionNameComponentName;
    static const QLatin1String s_guiContextComponentName;
    static const QLatin1String s_transactionNameComponentName;
    static const QLatin1String s_currentContentSetsComponentName;
    static const QLatin1String s_availableContentSetsComponentName;
    static const QLatin1String loggedComponentsComponentName;

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

    VeinLogger::DatabaseLogger *m_qPtr = nullptr;
    friend class DatabaseLogger;
};

#endif // DATALOGGERPRIVATE_H
