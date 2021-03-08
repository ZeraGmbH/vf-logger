#include "vl_databaseloggerprivate.h"

using namespace VeinLogger;

DataLoggerPrivate::DataLoggerPrivate(DatabaseLogger *t_qPtr) : m_qPtr(t_qPtr)
{
    m_batchedExecutionTimer.setInterval(5000);
    m_batchedExecutionTimer.setSingleShot(false);
}
DataLoggerPrivate::~DataLoggerPrivate()
{
    m_batchedExecutionTimer.stop();
    if(m_database != nullptr)
    {
        m_database->deleteLater(); ///@todo: check if the delete works across threads
        m_database = nullptr;
    }
    m_asyncDatabaseThread.quit();
    m_asyncDatabaseThread.wait();
}

void DataLoggerPrivate::initOnce() {
    // Q_ASSERT(m_initDone == false);
    if(m_initDone == false) {
        initStateMachine();
        m_initDone = true;
    }
}

void DataLoggerPrivate::setStatusText(const QString &t_status)
{
    if(m_loggerStatusText != t_status) {
        m_loggerStatusText = t_status;
        m_qPtr->m_loggingStatus=t_status;
    }
}

// We need a state machine because we are working with timers. We want t start those timers inside threaded RPCs.
// Qt does not support to start timers in threads without eventloop.
void DataLoggerPrivate::initStateMachine()
{
    m_stateMachine.setInitialState(m_databaseUninitializedState);
    //uninitilized -> ready
    m_databaseUninitializedState->addTransition(m_qPtr, &DatabaseLogger::sigDatabaseReady, m_loggingDisabledState);
    //ready -> uninitilized
    m_loggingDisabledState->addTransition(m_qPtr, &DatabaseLogger::sigDatabaseUnloaded, m_databaseUninitializedState);
    //ready -> enabled
    m_loggingDisabledState->addTransition(m_qPtr, &DatabaseLogger::sigLoggingStarted, m_loggingEnabledState);
    //enabled -> ready
    m_loggingEnabledState->addTransition(m_qPtr, &DatabaseLogger::sigLoggingStopped, m_loggingDisabledState);
    //enabled -> uninitilized
    m_loggingEnabledState->addTransition(m_qPtr, &DatabaseLogger::sigDatabaseUnloaded, m_databaseUninitializedState);


    QObject::connect(m_databaseUninitializedState, &QState::entered, [&]() {
        m_deleteWatcher.removePaths(m_deleteWatcher.directories());
        m_qPtr->m_databaseReady=false;
        m_qPtr->m_loggingEnabled=false;
        m_qPtr->m_databaseFileMimeType=QString();
        m_qPtr->m_databaseFileSize=qlonglong();
        m_qPtr->m_filesystemInfo=QVariantMap();
        m_qPtr->m_scheduledLoggingEnabled=false;
        m_qPtr->m_scheduledLoggingCountdown=0;
        m_batchedExecutionTimer.stop();
        m_qPtr->setLoggingEnabled(false);
        if(!m_noUninitMessage) {
            setStatusText("No database selected");
        }
    });


    QObject::connect(m_databaseUninitializedState, &QState::exited, [&]() {
        QHash <QString, QVariant> fileInfoData;
        QFileInfo fileInfo(m_databaseFilePath);
        QMimeDatabase mimeDB;
        m_qPtr->m_databaseFileMimeType=mimeDB.mimeTypeForFile(fileInfo, QMimeDatabase::MatchContent).name();
        m_qPtr->m_databaseFileSize=fileInfo.size();
        QStorageInfo storageInfo(fileInfo.absolutePath());
        // * To avoid fire storm on logging we watch file's dir
        // * For removable devices: mount-point's parent dir
        QStringList watchedPaths;
        watchedPaths.append(fileInfo.absolutePath());
        if(!storageInfo.isRoot()) {
            QDir tmpDir(storageInfo.rootPath());
            tmpDir.cdUp();
            watchedPaths.append(tmpDir.path());
        }
        qInfo("Database logger watching path(s): %s", qPrintable(watchedPaths.join(QStringLiteral(" + "))));
        QStringList unWatchedPaths = m_deleteWatcher.addPaths(watchedPaths);
        if(m_deleteWatcher.directories().count()) {
            QObject::connect(&m_deleteWatcher, &QFileSystemWatcher::directoryChanged, m_qPtr, &DatabaseLogger::checkDatabaseStillValid);
        }
        if(unWatchedPaths.count()) {
            qWarning("Unwatched paths: %s", qPrintable(unWatchedPaths.join(QStringLiteral(" + "))));
        }
    });

    QObject::connect(m_loggingDisabledState, &QState::entered, [&](){
        // Now we have an open and valid database: Notify ready and some
        // bits we were not sure to have at openDatabase
        m_qPtr->m_databaseReady=true;
        m_qPtr->m_loggingEnabled=false;
        m_qPtr->m_scheduledLoggingEnabled=false;
        m_batchedExecutionTimer.stop();
        m_qPtr->setLoggingEnabled(false);
        setStatusText("Database loaded");
        updateDBFileSizeInfo();
    });
    QObject::connect(m_loggingEnabledState, &QState::entered, [&](){
        m_qPtr->m_loggingEnabled=true;
        m_batchedExecutionTimer.start();
        if(m_qPtr->m_scheduledLoggingEnabled == true){
            m_schedulingTimer.start();
            m_countdownUpdateTimer.start();
        }
        setStatusText("Logging data");
    });

    QObject::connect(m_loggingEnabledState, &QState::exited, [&](){
        m_schedulingTimer.stop();
        m_countdownUpdateTimer.stop();
        m_qPtr->m_scheduledLoggingCountdown=0;
        emit m_qPtr->sigSingleShot();
    });



    m_stateMachine.start();
}

void DataLoggerPrivate::updateDBStorageInfo()
{
    const auto storages = QStorageInfo::mountedVolumes();
    QVariantMap storageInfoMap;
    for(const auto storDevice : storages)  {
        if(storDevice.fileSystemType().contains("tmpfs") == false) {
            const double availGB = storDevice.bytesFree()/1.0e9;
            const double totalGB = storDevice.bytesTotal()/1.0e9;

            QVariantMap storageData;
            storageData.insert("FilesystemFree", availGB);
            storageData.insert("FilesystemTotal", totalGB);

            storageInfoMap.insert(storDevice.rootPath(), storageData);
        }
    }
    m_qPtr->m_filesystemInfo=storageInfoMap;
}

bool DataLoggerPrivate::checkDBFilePath(const QString &t_dbFilePath)
{
    bool retVal = false;
    QFileInfo fInfo(t_dbFilePath);

    if(!fInfo.isRelative()) {
        // try to create path
        if(!fInfo.absoluteDir().exists()) {
            QDir dir;
            dir.mkpath(fInfo.absoluteDir().path());
        }

        if(fInfo.absoluteDir().exists()) {
            if(fInfo.isFile() || fInfo.exists() == false) {
                retVal = true;
            }
            else {
                emit m_qPtr->sigDatabaseError(QString("Path is not a valid file location: %1").arg(t_dbFilePath));
            }
        }
        else {
            emit m_qPtr->sigDatabaseError(QString("Parent directory for path does not exist: %1").arg(t_dbFilePath));
        }
    }
    else {
        emit m_qPtr->sigDatabaseError(QString("Relative paths are not accepted: %1").arg(t_dbFilePath));
    }

    return retVal;
}

void DataLoggerPrivate::updateDBFileSizeInfo()
{
    QFileInfo fInfo(m_databaseFilePath);
    if(fInfo.exists()) {
        m_qPtr->m_databaseFileSize=fInfo.size();
    }
}

void DataLoggerPrivate::updateSchedulerCountdown()
{
    if(m_schedulingTimer.isActive()) {
        m_qPtr->m_scheduledLoggingCountdown=m_schedulingTimer.remainingTime();
    }
}




