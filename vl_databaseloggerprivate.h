#ifndef VL_DATABASELOGGERPRIVATE_H
#define VL_DATABASELOGGERPRIVATE_H

#include <QThread>
#include <QTimer>
#include <QStateMachine>
#include <QStorageInfo>
#include <QFileSystemWatcher>
#include <QMimeDatabase>

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
    QFileSystemWatcher m_deleteWatcher;
    bool m_noUninitMessage = false;

    QTimer m_schedulingTimer;
    QTimer m_countdownUpdateTimer;
    bool m_initDone=false;
    QString m_loggerStatusText="Logging inactive";
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


    QStateMachine m_stateMachine;
    //QStatemachine does not support ParallelState
    //Therefore we add one state where the actual statemachine starts
    QState *m_databaseUninitializedState = new QState(&m_stateMachine);
    QState *m_loggingDisabledState = new QState(&m_stateMachine);
    QState *m_loggingEnabledState = new QState(&m_stateMachine);


    AbstractLoggerDB::STORAGE_MODE m_storageMode;

    DatabaseLogger *m_qPtr=nullptr;
    friend class DatabaseLogger;
};


}

#endif // VL_DATABASELOGGERPRIVATE_H
