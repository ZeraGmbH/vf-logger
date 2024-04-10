#include "vl_databaselogger.h"
#include "vl_datasource.h"
#include "vl_qmllogger.h"

#include <QHash>
#include <QThread>
#include <QTimer>
#include <QStateMachine>
#include <QStorageInfo>
#include <QMimeDatabase>
#include <QFileSystemWatcher>

#include <ve_commandevent.h>
#include <vcmp_componentdata.h>
#include <vcmp_entitydata.h>
#include <vcmp_errordata.h>
#include <vf-cpp-rpc.h>
#include <QJsonDocument>

Q_LOGGING_CATEGORY(VEIN_LOGGER, VEIN_DEBUGNAME_LOGGER)

namespace VeinLogger
{
class DataLoggerPrivate: public QObject
{
    explicit DataLoggerPrivate(DatabaseLogger *t_qPtr) : m_qPtr(t_qPtr)
    {
        m_batchedExecutionTimer.setInterval(5000);
        m_batchedExecutionTimer.setSingleShot(false);
    }
    ~DataLoggerPrivate()
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

    void initOnce() {
        Q_ASSERT(m_initDone == false);
        if(m_initDone == false) {
            VeinComponent::EntityData *systemData = new VeinComponent::EntityData();
            systemData->setCommand(VeinComponent::EntityData::Command::ECMD_ADD);
            systemData->setEntityId(m_entityId);

            VeinEvent::CommandEvent *systemEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, systemData);

            emit m_qPtr->sigSendEvent(systemEvent);

            VeinComponent::ComponentData *initialData = nullptr;

            QHash<QString, QVariant> componentData;
            componentData.insert(s_entityNameComponentName, m_entityName);
            componentData.insert(s_loggingEnabledComponentName, QVariant(false));
            componentData.insert(s_loggingStatusTextComponentName, QVariant(QString("Logging inactive")));
            ///@todo load from persistent settings file?
            componentData.insert(s_databaseReadyComponentName, QVariant(false));
            componentData.insert(s_databaseFileComponentName, QVariant(QString()));
            componentData.insert(s_databaseErrorFileComponentName, QVariant(QString()));
            componentData.insert(s_databaseFileSizeComponentName, QVariant(QString()));
            componentData.insert(s_scheduledLoggingEnabledComponentName, QVariant(false));
            componentData.insert(s_scheduledLoggingDurationComponentName, QVariant());
            componentData.insert(s_scheduledLoggingCountdownComponentName, QVariant(0.0));
            componentData.insert(s_existingSessionsComponentName, QStringList());
            componentData.insert(s_customerDataComponentName, QString());

            // TODO: Add more from modulemanager
            componentData.insert(s_sessionNameComponentName, QString());
            componentData.insert(s_guiContextComponentName, QString());
            componentData.insert(s_transactionNameComponentName, QString());
            componentData.insert(s_currentContentSetsComponentName, QStringList());
            componentData.insert(s_availableContentSetsComponentName, QStringList());

            for(const QString &componentName : componentData.keys()) {
                initialData = new VeinComponent::ComponentData();
                initialData->setEntityId(m_entityId);
                initialData->setCommand(VeinComponent::ComponentData::Command::CCMD_ADD);
                initialData->setComponentName(componentName);
                initialData->setNewValue(componentData.value(componentName));
                initialData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
                initialData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

                systemEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, initialData);
                emit m_qPtr->sigSendEvent(systemEvent);
            }

            QMap<QString,QString> tmpParamMap;
            VfCpp::cVeinModuleRpc::Ptr tmpval= VfCpp::cVeinModuleRpc::Ptr(new VfCpp::cVeinModuleRpc(m_entityId,m_qPtr,m_qPtr,"RPC_readTransaction",VfCpp::cVeinModuleRpc::Param({{"p_session", "QString"},{"p_transaction", "QString"}})), &QObject::deleteLater);
            m_rpcList[tmpval->rpcName()]=tmpval;
            tmpval= VfCpp::cVeinModuleRpc::Ptr(new VfCpp::cVeinModuleRpc(m_entityId,m_qPtr,m_qPtr,"RPC_readSessionComponent",VfCpp::cVeinModuleRpc::Param({{"p_session", "QString"},{"p_entity", "QString"},{"p_component", "QString"}})), &QObject::deleteLater);
            m_rpcList[tmpval->rpcName()]=tmpval;
            tmpval= VfCpp::cVeinModuleRpc::Ptr(new VfCpp::cVeinModuleRpc(m_entityId,m_qPtr,m_qPtr,"RPC_deleteSession",VfCpp::cVeinModuleRpc::Param({{"p_session", "QString"}})), &QObject::deleteLater);
            m_rpcList[tmpval->rpcName()]=tmpval;


            initStateMachine();

            m_initDone = true;
        }
    }

    void setStatusText(const QString &t_status)
    {
        if(m_loggerStatusText != t_status) {
            m_loggerStatusText = t_status;

            VeinComponent::ComponentData *schedulingEnabledData = new VeinComponent::ComponentData();
            schedulingEnabledData->setEntityId(m_entityId);
            schedulingEnabledData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
            schedulingEnabledData->setComponentName(DataLoggerPrivate::s_loggingStatusTextComponentName);
            schedulingEnabledData->setNewValue(t_status);
            schedulingEnabledData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
            schedulingEnabledData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

            emit m_qPtr->sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, schedulingEnabledData));
        }
    }

    void initStateMachine()
    {
        m_parallelWrapperState->setChildMode(QStateMachine::ParallelStates);
        m_stateMachine.setInitialState(m_parallelWrapperState);
        m_databaseContainerState->setInitialState(m_databaseUninitializedState);
        m_loggingContainerState->setInitialState(m_loggingDisabledState);
        m_logSchedulerContainerState->setInitialState(m_logSchedulerDisabledState);

        //uninitialized -> ready
        m_databaseUninitializedState->addTransition(m_qPtr, &DatabaseLogger::sigDatabaseReady, m_databaseReadyState);
        //ready -> uninitialized
        m_databaseReadyState->addTransition(m_qPtr, &DatabaseLogger::sigDatabaseUnloaded, m_databaseUninitializedState);

        //enabled -> disabled
        m_loggingEnabledState->addTransition(m_qPtr, &DatabaseLogger::sigLoggingStopped, m_loggingDisabledState);
        //disabled -> enabled
        m_loggingDisabledState->addTransition(m_qPtr, &DatabaseLogger::sigLoggingStarted, m_loggingEnabledState);

        //enabled -> disbled
        m_logSchedulerEnabledState->addTransition(m_qPtr, &DatabaseLogger::sigLogSchedulerDeactivated, m_logSchedulerDisabledState);
        //disabled -> enabled
        m_logSchedulerDisabledState->addTransition(m_qPtr, &DatabaseLogger::sigLogSchedulerActivated, m_logSchedulerEnabledState);

        QObject::connect(m_databaseUninitializedState, &QState::entered, [&]() {
            VeinComponent::ComponentData *databaseUninitializedCData = new VeinComponent::ComponentData();
            databaseUninitializedCData->setEntityId(m_entityId);
            databaseUninitializedCData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
            databaseUninitializedCData->setComponentName(DataLoggerPrivate::s_databaseReadyComponentName);
            databaseUninitializedCData->setNewValue(false);
            databaseUninitializedCData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
            databaseUninitializedCData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

            emit m_qPtr->sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, databaseUninitializedCData));
            m_qPtr->setLoggingEnabled(false);
            if(!m_noUninitMessage) {
                setStatusText("No database selected");
            }
        });
        QObject::connect(m_databaseReadyState, &QState::entered, [&](){
            // Now we have an open and valid database: Notify ready and some
            // bits we were not sure to have at openDatabase
            QHash <QString, QVariant> fileInfoData;
            QFileInfo fileInfo(m_databaseFilePath);
            fileInfoData.insert(DataLoggerPrivate::s_databaseFileSizeComponentName, fileInfo.size());
            fileInfoData.insert(DataLoggerPrivate::s_databaseReadyComponentName, true);
            for(const QString &componentName : fileInfoData.keys())  {
                VeinComponent::ComponentData *storageCData = new VeinComponent::ComponentData();
                storageCData->setEntityId(m_entityId);
                storageCData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
                storageCData->setComponentName(componentName);
                storageCData->setNewValue(fileInfoData.value(componentName));
                storageCData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
                storageCData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);
                emit m_qPtr->sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, storageCData));
            }

            m_qPtr->setLoggingEnabled(false);
            setStatusText("Database loaded");

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
        QObject::connect(m_loggingEnabledState, &QState::entered, [&](){
            setStatusText("Logging data");
        });
        QObject::connect(m_loggingDisabledState, &QState::entered, [&](){
            if(!m_noUninitMessage) {
                // yes we are in logging disabled state - but the message 'Logging disabled' is a
                // bit misleading: sounds as something is wrong and blocking further logging
                //setStatusText("Logging disabled");
                setStatusText("Database loaded");
            }
            m_batchedExecutionTimer.stop();
            updateDBFileSizeInfo();
        });
        QObject::connect(m_logSchedulerEnabledState, &QState::entered, [&](){
            VeinComponent::ComponentData *schedulingEnabledCData = new VeinComponent::ComponentData();
            schedulingEnabledCData->setEntityId(m_entityId);
            schedulingEnabledCData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
            schedulingEnabledCData->setComponentName(DataLoggerPrivate::s_scheduledLoggingEnabledComponentName);
            schedulingEnabledCData->setNewValue(true);
            schedulingEnabledCData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
            schedulingEnabledCData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

            emit m_qPtr->sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, schedulingEnabledCData));
        });
        QObject::connect(m_logSchedulerDisabledState, &QState::entered, [&](){
            VeinComponent::ComponentData *schedulingDisabledCData = new VeinComponent::ComponentData();
            schedulingDisabledCData->setEntityId(m_entityId);
            schedulingDisabledCData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
            schedulingDisabledCData->setComponentName(DataLoggerPrivate::s_scheduledLoggingEnabledComponentName);
            schedulingDisabledCData->setNewValue(false);
            schedulingDisabledCData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
            schedulingDisabledCData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

            emit m_qPtr->sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, schedulingDisabledCData));
        });

        m_stateMachine.start();
    }

    bool checkDBFilePath(const QString &t_dbFilePath)
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

    void updateDBFileSizeInfo()
    {
        QFileInfo fInfo(m_databaseFilePath);
        if(fInfo.exists()) {
            VeinComponent::ComponentData *storageCData = new VeinComponent::ComponentData();
            storageCData->setEntityId(m_entityId);
            storageCData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
            storageCData->setComponentName(DataLoggerPrivate::s_databaseFileSizeComponentName);
            storageCData->setNewValue(QVariant(fInfo.size()));
            storageCData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
            storageCData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

            emit m_qPtr->sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, storageCData));
        }
    }

    void updateSchedulerCountdown()
    {
        if(m_schedulingTimer.isActive()) {
            VeinComponent::ComponentData *schedulerCountdownCData = new VeinComponent::ComponentData();
            schedulerCountdownCData->setEntityId(m_entityId);
            schedulerCountdownCData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
            schedulerCountdownCData->setComponentName(DataLoggerPrivate::s_scheduledLoggingCountdownComponentName);
            schedulerCountdownCData->setNewValue(QVariant(m_schedulingTimer.remainingTime()));
            schedulerCountdownCData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
            schedulerCountdownCData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

            emit m_qPtr->sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, schedulerCountdownCData));
        }
    }

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
    static const QLatin1String s_entityNameComponentName;
    static const QLatin1String s_loggingStatusTextComponentName;
    static const QLatin1String s_loggingEnabledComponentName;
    static const QLatin1String s_databaseReadyComponentName;
    static const QLatin1String s_databaseFileComponentName;
    static const QLatin1String s_databaseErrorFileComponentName;
    static const QLatin1String s_databaseFileSizeComponentName;
    static const QLatin1String s_scheduledLoggingEnabledComponentName;
    static const QLatin1String s_scheduledLoggingDurationComponentName;
    static const QLatin1String s_scheduledLoggingCountdownComponentName;
    static const QLatin1String s_existingSessionsComponentName;

    // TODO: Add more from modulemanager
    static const QLatin1String s_customerDataComponentName;
    static const QLatin1String s_sessionNameComponentName;
    static const QLatin1String s_guiContextComponentName;
    static const QLatin1String s_transactionNameComponentName;
    static const QLatin1String s_currentContentSetsComponentName;
    static const QLatin1String s_availableContentSetsComponentName;

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

const QLatin1String DataLoggerPrivate::s_entityNameComponentName = QLatin1String("EntityName");
const QLatin1String DataLoggerPrivate::s_loggingStatusTextComponentName  = QLatin1String("LoggingStatus");
const QLatin1String DataLoggerPrivate::s_loggingEnabledComponentName = QLatin1String("LoggingEnabled");
const QLatin1String DataLoggerPrivate::s_databaseReadyComponentName = QLatin1String("DatabaseReady");
const QLatin1String DataLoggerPrivate::s_databaseFileComponentName = QLatin1String("DatabaseFile");
const QLatin1String DataLoggerPrivate::s_databaseErrorFileComponentName = QLatin1String("DatabaseErrorFile");
const QLatin1String DataLoggerPrivate::s_databaseFileSizeComponentName = QLatin1String("DatabaseFileSize");
const QLatin1String DataLoggerPrivate::s_scheduledLoggingEnabledComponentName = QLatin1String("ScheduledLoggingEnabled");
const QLatin1String DataLoggerPrivate::s_scheduledLoggingDurationComponentName = QLatin1String("ScheduledLoggingDuration");
const QLatin1String DataLoggerPrivate::s_scheduledLoggingCountdownComponentName = QLatin1String("ScheduledLoggingCountdown");
const QLatin1String DataLoggerPrivate::s_existingSessionsComponentName = QLatin1String("ExistingSessions");
// TODO: Add more from modulemanager
const QLatin1String DataLoggerPrivate::s_customerDataComponentName = QLatin1String("CustomerData");
const QLatin1String DataLoggerPrivate::s_sessionNameComponentName = QLatin1String("sessionName");
const QLatin1String DataLoggerPrivate::s_guiContextComponentName = QLatin1String("guiContext");
const QLatin1String DataLoggerPrivate::s_transactionNameComponentName = QLatin1String("transactionName");
const QLatin1String DataLoggerPrivate::s_currentContentSetsComponentName = QLatin1String("currentContentSets");
const QLatin1String DataLoggerPrivate::s_availableContentSetsComponentName = QLatin1String("availableContentSets");

DatabaseLogger::DatabaseLogger(DataSource *t_dataSource, DBFactory t_factoryFunction, QObject *t_parent, AbstractLoggerDB::STORAGE_MODE t_storageMode) :
    VeinEvent::EventSystem(t_parent),
    m_dPtr(new DataLoggerPrivate(this))
{
    m_dPtr->m_dataSource=t_dataSource;
    m_dPtr->m_asyncDatabaseThread.setObjectName("VFLoggerDBThread");
    m_dPtr->m_schedulingTimer.setSingleShot(true);
    m_dPtr->m_countdownUpdateTimer.setInterval(100);
    m_dPtr->m_databaseFactory = t_factoryFunction;
    m_dPtr->m_storageMode=t_storageMode;
    switch(t_storageMode) {
    case AbstractLoggerDB::STORAGE_MODE::TEXT: {
        m_dPtr->m_entityId = 2;
        m_dPtr->m_entityName = QLatin1String("_LoggingSystem");
        qCDebug(VEIN_LOGGER) << "Created plaintext logger:" << m_dPtr->m_entityName << "with id:" << m_dPtr->m_entityId;
        break;
    }
    case AbstractLoggerDB::STORAGE_MODE::BINARY: {
        //use different id and entity name
        m_dPtr->m_entityId = 200000;
        m_dPtr->m_entityName = QLatin1String("_BinaryLoggingSystem");
        qCDebug(VEIN_LOGGER) << "Created binary logger:" << m_dPtr->m_entityName << "with id:" << m_dPtr->m_entityId;
        break;
    }
    }

    connect(this, &DatabaseLogger::sigAttached, [this](){ m_dPtr->initOnce(); });
    connect(&m_dPtr->m_batchedExecutionTimer, &QTimer::timeout, [this]() {
        m_dPtr->updateDBFileSizeInfo();
        if(m_dPtr->m_stateMachine.configuration().contains(m_dPtr->m_loggingDisabledState)) {
            m_dPtr->m_batchedExecutionTimer.stop();
        }
    });
    connect(&m_dPtr->m_schedulingTimer, &QTimer::timeout, [this]() {
        setLoggingEnabled(false);
    });

    connect(&m_dPtr->m_countdownUpdateTimer, &QTimer::timeout, [this]() {
        m_dPtr->updateSchedulerCountdown();
    });

    connect(this, &DatabaseLogger::sigLoggingEnabledChanged, [this](bool t_enabled) {
        VeinComponent::ComponentData *loggingEnabledCData = new VeinComponent::ComponentData();
        loggingEnabledCData->setEntityId(m_dPtr->m_entityId);
        loggingEnabledCData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
        loggingEnabledCData->setComponentName(DataLoggerPrivate::s_loggingEnabledComponentName);
        loggingEnabledCData->setNewValue(t_enabled);
        loggingEnabledCData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
        loggingEnabledCData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

        emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, loggingEnabledCData));
    });

    // db error handling
    connect(this, &DatabaseLogger::sigDatabaseError, [this](const QString &t_errorString) {
        qCWarning(VEIN_LOGGER) << t_errorString;

        // error db filename notification
        VeinComponent::ComponentData *dbErrorFileNameCData = new VeinComponent::ComponentData();
        dbErrorFileNameCData->setEntityId(m_dPtr->m_entityId);
        dbErrorFileNameCData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
        dbErrorFileNameCData->setComponentName(DataLoggerPrivate::s_databaseErrorFileComponentName);
        dbErrorFileNameCData->setNewValue(m_dPtr->m_databaseFilePath);
        dbErrorFileNameCData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
        dbErrorFileNameCData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);
        emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, dbErrorFileNameCData));

        closeDatabase();
        m_dPtr->m_noUninitMessage = true;
        m_dPtr->setStatusText("Database error");
    });
}

DatabaseLogger::~DatabaseLogger()
{
    delete m_dPtr;
}

/**
 * @brief DatabaseLogger::addScript
 * @param t_script
 * @todo Make customer data system optional
 */
void DatabaseLogger::addScript(QmlLogger *t_script)
{
    const QSet<QAbstractState*> requiredStates = {m_dPtr->m_loggingEnabledState, m_dPtr->m_databaseReadyState};
    if(m_dPtr->m_stateMachine.configuration().contains(requiredStates) && m_dPtr->m_loggerScripts.contains(t_script) == false) {
        m_dPtr->m_loggerScripts.append(t_script);
        //writes the values from the data source to the database, some values may never change so they need to be initialized
        if(t_script->initializeValues() == true) {
            const QString tmpsessionName = t_script->sessionName();
            const QVector<QString> tmpTransactionName = {t_script->transactionName()};
            QString tmpContentSets = t_script->contentSets().join(QLatin1Char(','));
            //add a new transaction and store ids in script.
            t_script->setTransactionId(m_dPtr->m_database->addTransaction(t_script->transactionName(),t_script->sessionName(), tmpContentSets, t_script->guiContext()));
            const QVector<int> tmpTransactionIds = {t_script->getTransactionId()};
            // add starttime to transaction. stop time is set in batch execution.
            m_dPtr->m_database->addStartTime(t_script->getTransactionId(),QDateTime::currentDateTime());

            QMultiHash<int, QString> tmpLoggedValues = t_script->getLoggedValues();

            for(const int tmpEntityId : tmpLoggedValues.uniqueKeys()) { //only process once for every entity
                const QList<QString> tmpComponents = tmpLoggedValues.values(tmpEntityId);
                for(const QString &tmpComponentName : tmpComponents) {
                    if(m_dPtr->m_dataSource->hasEntity(tmpEntityId)) { // is entity available?
                        if(m_dPtr->m_database->hasEntityId(tmpEntityId) == false) { // already in db?
                            emit sigAddEntity(tmpEntityId, m_dPtr->m_dataSource->getEntityName(tmpEntityId));
                        }
                        QStringList componentNamesToAdd;
                        if(tmpComponentName == QStringLiteral("__ALL_COMPONENTS__")) {
                            componentNamesToAdd = m_dPtr->m_dataSource->getEntityComponentsForStore(tmpEntityId);
                        }
                        else {
                            componentNamesToAdd.append(tmpComponentName);
                        }
                        for (auto componentToAdd : componentNamesToAdd) {
                            // add component to db
                            if(m_dPtr->m_database->hasComponentName(componentToAdd) == false) {
                                emit sigAddComponent(componentToAdd);
                            }
                            // add initial values
                            emit sigAddLoggedValue(
                                        tmpsessionName,
                                        tmpTransactionIds,
                                        tmpEntityId,
                                        componentToAdd,
                                        m_dPtr->m_dataSource->getValue(tmpEntityId, componentToAdd),
                                        QDateTime::currentDateTime());
                        }
                    }
                }
            }
        }
    }
}

void DatabaseLogger::removeScript(QmlLogger *t_script)
{
    m_dPtr->m_loggerScripts.removeAll(t_script);

}

bool DatabaseLogger::loggingEnabled() const
{
    return m_dPtr->m_stateMachine.configuration().contains(m_dPtr->m_loggingEnabledState);
}

int DatabaseLogger::entityId() const
{
    return m_dPtr->m_entityId;
}

QString DatabaseLogger::entityName() const
{
    return m_dPtr->m_entityName;
}

void DatabaseLogger::setLoggingEnabled(bool t_enabled)
{
    //do not accept values that are already set
    const QSet<QAbstractState *> activeStates = m_dPtr->m_stateMachine.configuration();
    if(t_enabled != activeStates.contains(m_dPtr->m_loggingEnabledState) ) {
        if(t_enabled) {
            m_dPtr->m_batchedExecutionTimer.start();
            if(activeStates.contains(m_dPtr->m_logSchedulerEnabledState)) {
                m_dPtr->m_schedulingTimer.start();
                m_dPtr->m_countdownUpdateTimer.start();
            }
            emit sigLoggingStarted();
        }
        else {
            m_dPtr->m_schedulingTimer.stop();
            m_dPtr->m_countdownUpdateTimer.stop();
            emit sigLoggingStopped();
        }
        emit sigLoggingEnabledChanged(t_enabled);
    }
}

bool DatabaseLogger::openDatabase(const QString &t_filePath)
{
    m_dPtr->m_databaseFilePath = t_filePath;
    m_dPtr->m_noUninitMessage = false;
    // setup/init components
    QHash <QString, QVariant> fileInfoData;
    fileInfoData.insert(DataLoggerPrivate::s_databaseErrorFileComponentName, QString());
    fileInfoData.insert(DataLoggerPrivate::s_databaseFileComponentName, t_filePath);
    qint64 fileSize = 0;
    // Size is set (again) in database-ready - there we have a file definitely
    QFileInfo fileInfo(t_filePath);
    if(fileInfo.exists()) {
        fileSize = fileInfo.size();
    }
    fileInfoData.insert(DataLoggerPrivate::s_databaseFileSizeComponentName, fileSize);
    for(const QString &componentName : fileInfoData.keys())  {
        VeinComponent::ComponentData *storageCData = new VeinComponent::ComponentData();
        storageCData->setEntityId(m_dPtr->m_entityId);
        storageCData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
        storageCData->setComponentName(componentName);
        storageCData->setNewValue(fileInfoData.value(componentName));
        storageCData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
        storageCData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);
        emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, storageCData));
    }

    const bool validStorage = m_dPtr->checkDBFilePath(t_filePath); // throws sigDatabaseError on error
    if(validStorage == true) {
        if(m_dPtr->m_database != nullptr) {
            disconnect(m_dPtr->m_database, SIGNAL(sigDatabaseError(QString)), this, SIGNAL(sigDatabaseError(QString)));
            m_dPtr->m_database->deleteLater();
            m_dPtr->m_database = nullptr;
        }
        m_dPtr->m_asyncDatabaseThread.quit();
        m_dPtr->m_asyncDatabaseThread.wait();
        m_dPtr->m_database = m_dPtr->m_databaseFactory();//new SQLiteDB(t_storageMode);
        // forward database's error my handler
        connect(m_dPtr->m_database, SIGNAL(sigDatabaseError(QString)), this, SIGNAL(sigDatabaseError(QString)));
        m_dPtr->m_database->setStorageMode(m_dPtr->m_storageMode);
        m_dPtr->m_database->moveToThread(&m_dPtr->m_asyncDatabaseThread);
        m_dPtr->m_asyncDatabaseThread.start();

        // will be queued connection due to thread affinity
        connect(this, &DatabaseLogger::sigAddLoggedValue, m_dPtr->m_database, &AbstractLoggerDB::addLoggedValue);
        connect(this, SIGNAL(sigAddEntity(int, QString)), m_dPtr->m_database, SLOT(addEntity(int, QString)));
        connect(this, SIGNAL(sigAddComponent(QString)), m_dPtr->m_database, SLOT(addComponent(QString)));
        connect(this, SIGNAL(sigAddSession(QString,QList<QVariantMap>)), m_dPtr->m_database, SLOT(addSession(QString,QList<QVariantMap>)));
        connect(this, SIGNAL(sigOpenDatabase(QString)), m_dPtr->m_database, SLOT(openDatabase(QString)));
        connect(m_dPtr->m_database, SIGNAL(sigDatabaseReady()), this, SIGNAL(sigDatabaseReady()));
        connect(&m_dPtr->m_batchedExecutionTimer, SIGNAL(timeout()), m_dPtr->m_database, SLOT(runBatchedExecution()));
        // run final batch instantly when logging is disabled
        connect(m_dPtr->m_loggingDisabledState, SIGNAL(entered()), m_dPtr->m_database, SLOT(runBatchedExecution()));
        connect(m_dPtr->m_database, SIGNAL(sigNewSessionList(QStringList)), this, SLOT(updateSessionList(QStringList)));

        emit sigOpenDatabase(t_filePath);
    }
    return validStorage;
}

void DatabaseLogger::closeDatabase()
{
    m_dPtr->m_noUninitMessage = false;
    setLoggingEnabled(false);
    if(m_dPtr->m_database != nullptr) {
        disconnect(m_dPtr->m_database, SIGNAL(sigDatabaseError(QString)), this, SIGNAL(sigDatabaseError(QString)));
        m_dPtr->m_database->deleteLater();
        m_dPtr->m_database = nullptr;
    }
    m_dPtr->m_asyncDatabaseThread.quit();
    m_dPtr->m_asyncDatabaseThread.wait();
    if(m_dPtr->m_deleteWatcher.directories().count()) {
        QStringList watchedDirs = m_dPtr->m_deleteWatcher.directories();
        for(QString watchDir : watchedDirs) {
            m_dPtr->m_deleteWatcher.removePath(watchDir);
        }
        QObject::disconnect(&m_dPtr->m_deleteWatcher, &QFileSystemWatcher::directoryChanged, this, &DatabaseLogger::checkDatabaseStillValid);
    }
    emit sigDatabaseUnloaded();

    // set database file name empty
    QString closedDb = m_dPtr->m_databaseFilePath;
    m_dPtr->m_databaseFilePath.clear();
    VeinComponent::ComponentData *dbFileNameCData = new VeinComponent::ComponentData();
    dbFileNameCData->setEntityId(m_dPtr->m_entityId);
    dbFileNameCData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
    dbFileNameCData->setComponentName(DataLoggerPrivate::s_databaseFileComponentName);
    dbFileNameCData->setNewValue(QString());
    dbFileNameCData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
    dbFileNameCData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);
    emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, dbFileNameCData));

    updateSessionList(QStringList());

    // set CustomerData component empty on databaseClosed
    VeinComponent::ComponentData *customerCData = new VeinComponent::ComponentData();
    customerCData->setEntityId(m_dPtr->m_entityId);
    customerCData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
    customerCData->setComponentName(DataLoggerPrivate::s_customerDataComponentName);
    customerCData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
    customerCData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);
    customerCData->setNewValue(QString());
    emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, customerCData));

    qCDebug(VEIN_LOGGER) << "Unloaded database:" << closedDb;
}

void DatabaseLogger::checkDatabaseStillValid()
{
    QFile dbFile(m_dPtr->m_databaseFilePath);
    if(!dbFile.exists()) {
        emit sigDatabaseError(QString("Watcher detected database file %1 is gone!").arg(m_dPtr->m_databaseFilePath));
    }
}

void DatabaseLogger::updateSessionList(QStringList p_sessions)
{
    VeinComponent::ComponentData *exisitingSessions = new VeinComponent::ComponentData();
    exisitingSessions ->setEntityId(m_dPtr->m_entityId);
    exisitingSessions ->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
    exisitingSessions ->setComponentName(DataLoggerPrivate::s_existingSessionsComponentName);
    exisitingSessions ->setNewValue(p_sessions);
    exisitingSessions ->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
    exisitingSessions ->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);
    emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, exisitingSessions));

}

QVariant DatabaseLogger::RPC_deleteSession(QVariantMap p_parameters){
    QVariant retVal;
    QString session = p_parameters["p_session"].toString();
    retVal=m_dPtr->m_database->deleteSession(session);

    // check if deleted session is current Session and if it is set sessionName empty
    // We will not check retVal here. If something goes wrong and the session is still availabel the
    // user can choose it again without risking undefined behavior.
    if(session == m_dPtr->m_sessionName){
        VeinComponent::ComponentData *sessionNameCData = new VeinComponent::ComponentData();
        sessionNameCData ->setEntityId(m_dPtr->m_entityId);
        sessionNameCData ->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
        sessionNameCData ->setComponentName(DataLoggerPrivate::s_sessionNameComponentName);
        sessionNameCData ->setNewValue(QString());
        sessionNameCData ->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
        sessionNameCData ->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);
        m_dPtr->m_sessionName="";
        emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, sessionNameCData));
    }
    return retVal;
}

QVariant DatabaseLogger::RPC_readSessionComponent(QVariantMap p_parameters){
    QVariant retVal;
    QString session = p_parameters["p_session"].toString();
    QString entity = p_parameters["p_entity"].toString();
    QString component = p_parameters["p_component"].toString();
    retVal=m_dPtr->m_database->readSessionComponent(session,entity,component);
    return retVal;
}



QVariant DatabaseLogger::RPC_readTransaction(QVariantMap p_parameters){
    QString session = p_parameters["p_session"].toString();
    QString transaction = p_parameters["p_transaction"].toString();
    QJsonDocument retVal;
    if(m_dPtr->m_stateMachine.configuration().contains(m_dPtr->m_databaseReadyState)){
        retVal=m_dPtr->m_database->readTransaction(transaction,session);
    }
    return QVariant::fromValue(retVal.toJson());
}

void DatabaseLogger::processEvent(QEvent *t_event)
{
    using namespace VeinEvent;
    using namespace VeinComponent;
    if(t_event->type()==CommandEvent::getQEventType()) {
        CommandEvent *cEvent = nullptr;
        EventData *evData = nullptr;
        cEvent = static_cast<CommandEvent *>(t_event);
        Q_ASSERT(cEvent != nullptr);

        evData = cEvent->eventData();
        Q_ASSERT(evData != nullptr);

        const QSet<QAbstractState*> activeStates = m_dPtr->m_stateMachine.configuration();
        const QSet<QAbstractState*> requiredStates = {m_dPtr->m_loggingEnabledState, m_dPtr->m_databaseReadyState};
        if(evData->type()==ComponentData::dataType()) {

            ComponentData *cData=nullptr;
            cData = static_cast<ComponentData *>(evData);

            if(cEvent->eventSubtype() == CommandEvent::EventSubtype::NOTIFICATION) {

                Q_ASSERT(cData != nullptr);

                ///@todo check if the setLoggingEnabled() call can be moved to the transaction code block for s_loggingEnabledComponentName
                if(cData->entityId() == entityId() && cData->componentName() == DataLoggerPrivate::s_loggingEnabledComponentName) {
                    setLoggingEnabled(cData->newValue().toBool());
                }

                if(activeStates.contains(requiredStates)) {
                    QString sessionName = "";
                    QVector<int> transactionIds;
                    const QVector<QmlLogger *> scripts = m_dPtr->m_loggerScripts;
                    //check all scripts if they want to log the changed value
                    for(const QmlLogger *entry : scripts) {
                        if(entry->isLoggedComponent(evData->entityId(), cData->componentName())) {
                            sessionName = entry->sessionName();
                            transactionIds.append(entry->getTransactionId());
                        }
                    }

                    if(sessionName.length() > 0)
                    {
                        if(m_dPtr->m_database->hasSessionName(sessionName) == false) {
                            emit sigAddSession(sessionName,QList<QVariantMap>());
                        }
                        if(m_dPtr->m_database->hasEntityId(evData->entityId()) == false) {
                            emit sigAddEntity(evData->entityId(), m_dPtr->m_dataSource->getEntityName(cData->entityId()));
                        }
                        if(m_dPtr->m_database->hasComponentName(cData->componentName()) == false) {
                            emit sigAddComponent(cData->componentName());
                        }
                        if(transactionIds.length() != 0) {
                            emit sigAddLoggedValue(sessionName, transactionIds, cData->entityId(), cData->componentName(), cData->newValue(), QDateTime::currentDateTime());
                        }
                    }
                }
            }

            else if(cEvent->eventSubtype() == CommandEvent::EventSubtype::TRANSACTION &&
                    evData->entityId() == m_dPtr->m_entityId) {
                Q_ASSERT(cData != nullptr);

                if(cData->eventCommand() == VeinComponent::ComponentData::Command::CCMD_SET) {
                    if(cData->componentName() == DataLoggerPrivate::s_databaseFileComponentName) {
                        if(m_dPtr->m_database == nullptr || cData->newValue() != m_dPtr->m_databaseFilePath) {
                            if(cData->newValue().toString().isEmpty()) { //unsetting the file component = closing the database
                                closeDatabase();
                            }
                            else {
                                openDatabase(cData->newValue().toString());
                            }
                        }

                        // a good place to reset selected sessionName - however db-open ends up with
                        VeinComponent::ComponentData *sessionNameCData = new VeinComponent::ComponentData();
                        sessionNameCData ->setEntityId(m_dPtr->m_entityId);
                        sessionNameCData ->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
                        sessionNameCData ->setComponentName(DataLoggerPrivate::s_sessionNameComponentName);
                        sessionNameCData ->setNewValue(QString());
                        sessionNameCData ->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
                        sessionNameCData ->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);
                        m_dPtr->m_sessionName="";
                        emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, sessionNameCData));

                    }
                    else if(cData->componentName() == DataLoggerPrivate::s_loggingEnabledComponentName) {
                        VeinComponent::ComponentData *loggingEnabledCData = new VeinComponent::ComponentData();
                        loggingEnabledCData->setEntityId(m_dPtr->m_entityId);
                        loggingEnabledCData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
                        loggingEnabledCData->setComponentName(DataLoggerPrivate::s_loggingEnabledComponentName);
                        loggingEnabledCData->setNewValue(cData->newValue());
                        loggingEnabledCData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
                        loggingEnabledCData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

                        emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, loggingEnabledCData));
                    }
                    else if(cData->componentName() == DataLoggerPrivate::s_scheduledLoggingEnabledComponentName) {
                        //do not accept values that are already set
                        if(cData->newValue().toBool() != m_dPtr->m_stateMachine.configuration().contains(m_dPtr->m_logSchedulerEnabledState)) {
                            if(cData->newValue().toBool() == true) {
                                emit sigLogSchedulerActivated();
                            }
                            else {
                                emit sigLogSchedulerDeactivated();
                            }
                            setLoggingEnabled(false);
                        }
                    }
                    else if(cData->componentName() == DataLoggerPrivate::s_scheduledLoggingDurationComponentName) {
                        bool invalidTime = false;
                        bool conversionOk = false;
                        const int logDurationMsecs = cData->newValue().toInt(&conversionOk);
                        invalidTime = !conversionOk;

                        if(conversionOk == true && logDurationMsecs != m_dPtr->m_scheduledLoggingDuration) {
                            m_dPtr->m_scheduledLoggingDuration = logDurationMsecs;
                            if(logDurationMsecs > 0) {
                                m_dPtr->m_schedulingTimer.setInterval(logDurationMsecs);
                                if(activeStates.contains(requiredStates)) {
                                    m_dPtr->m_schedulingTimer.start(); //restart timer
                                }
                                VeinComponent::ComponentData *schedulingDurationData = new VeinComponent::ComponentData();
                                schedulingDurationData->setEntityId(m_dPtr->m_entityId);
                                schedulingDurationData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
                                schedulingDurationData->setComponentName(DataLoggerPrivate::s_scheduledLoggingDurationComponentName);
                                schedulingDurationData->setNewValue(cData->newValue());
                                schedulingDurationData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
                                schedulingDurationData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

                                emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, schedulingDurationData));
                            }
                            else {
                                invalidTime = true;
                            }
                        }
                        if(invalidTime) {
                            VeinComponent::ErrorData *errData = new VeinComponent::ErrorData();
                            errData->setEntityId(m_dPtr->m_entityId);
                            errData->setOriginalData(cData);
                            errData->setEventOrigin(VeinComponent::ErrorData::EventOrigin::EO_LOCAL);
                            errData->setEventTarget(VeinComponent::ErrorData::EventTarget::ET_ALL);
                            errData->setErrorDescription(QString("Invalid logging duration: %1").arg(cData->newValue().toString()));

                            sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, errData));
                        }
                    }
                    // TODO: Add more from modulemanager
                    else if(cData->componentName() == DataLoggerPrivate::s_sessionNameComponentName) {
                        VeinComponent::ComponentData *sessionNameCData = new VeinComponent::ComponentData();
                        sessionNameCData->setEntityId(m_dPtr->m_entityId);
                        sessionNameCData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
                        sessionNameCData->setComponentName(DataLoggerPrivate::s_sessionNameComponentName);
                        sessionNameCData->setNewValue(cData->newValue());
                        sessionNameCData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
                        sessionNameCData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

                        m_dPtr->m_sessionName=cData->newValue().toString();

                        VeinComponent::ComponentData *customerCData = new VeinComponent::ComponentData();
                        customerCData->setEntityId(m_dPtr->m_entityId);
                        customerCData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
                        customerCData->setComponentName(DataLoggerPrivate::s_customerDataComponentName);
                        customerCData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
                        customerCData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);



                        QString sessionName = cData->newValue().toString();
                        // we have a working database?
                        if(!sessionName.isEmpty() &&
                                m_dPtr->m_database &&
                                m_dPtr->m_database->databaseIsOpen()){

                            if(!m_dPtr->m_database->hasSessionName(sessionName)) {
                                // Add session immediately: That helps us massively to create a smart user-interface

                                QMultiHash<int, QString> tmpStaticComps;
                                QList<QVariantMap> tmpStaticData;

                                // Add customer data at the beginning
                                if(m_dPtr->m_dataSource->hasEntity(200)) {
                                    for(QString comp : m_dPtr->m_dataSource->getEntityComponentsForStore(200)){
                                        tmpStaticComps.insert(200,comp);
                                    }
                                }
                                // Add status module data at the beginning
                                if(m_dPtr->m_dataSource->hasEntity(1150)) {

                                    for(QString comp : m_dPtr->m_dataSource->getEntityComponentsForStore(1150)){
                                        tmpStaticComps.insert(1150,comp);
                                    }
                                }

                                for(const int tmpEntityId : tmpStaticComps.uniqueKeys()) { //only process once for every entity
                                    if(m_dPtr->m_database->hasEntityId(tmpEntityId) == false) { // already in db?
                                        emit sigAddEntity(tmpEntityId, m_dPtr->m_dataSource->getEntityName(tmpEntityId));
                                    }
                                    const QList<QString> tmpComponents = tmpStaticComps.values(tmpEntityId);
                                    for(const QString &tmpComponentName : tmpComponents) {
                                        if(m_dPtr->m_database->hasComponentName(tmpComponentName) == false) {
                                            emit sigAddComponent(tmpComponentName);
                                        }
                                        QVariantMap tmpMap;
                                        tmpMap["entityId"]=tmpEntityId;
                                        tmpMap["compName"]=tmpComponentName;
                                        tmpMap["value"]=m_dPtr->m_dataSource->getValue(tmpEntityId, tmpComponentName);
                                        tmpMap["time"]=QDateTime::currentDateTime();
                                        tmpStaticData.append(tmpMap);
                                    }
                                }

                                // We are reading it like this because it is faster than writing it to the db and then reading it agian
                                customerCData->setNewValue(m_dPtr->m_dataSource->getValue(200, "FileSelected"));
                                emit sigAddSession(sessionName,tmpStaticData);
                            }else{
                                QString customerdata=m_dPtr->m_database->readSessionComponent(sessionName,"CustomerData","FileSelected").toString();
                                customerCData->setNewValue(customerdata);
                            }

                        }

                        emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, sessionNameCData));
                        emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, customerCData));
                    }
                    else if(cData->componentName() == DataLoggerPrivate::s_guiContextComponentName) {
                        VeinComponent::ComponentData *guiContextCData = new VeinComponent::ComponentData();
                        guiContextCData->setEntityId(m_dPtr->m_entityId);
                        guiContextCData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
                        guiContextCData->setComponentName(DataLoggerPrivate::s_guiContextComponentName);
                        guiContextCData->setNewValue(cData->newValue());
                        guiContextCData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
                        guiContextCData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

                        emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, guiContextCData));
                    }
                    else if(cData->componentName() == DataLoggerPrivate::s_transactionNameComponentName) {
                        VeinComponent::ComponentData *transactionNameCData = new VeinComponent::ComponentData();
                        transactionNameCData->setEntityId(m_dPtr->m_entityId);
                        transactionNameCData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
                        transactionNameCData->setComponentName(DataLoggerPrivate::s_transactionNameComponentName);
                        transactionNameCData->setNewValue(cData->newValue());
                        transactionNameCData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
                        transactionNameCData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

                        emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, transactionNameCData));
                    }
                    else if(cData->componentName() == DataLoggerPrivate::s_currentContentSetsComponentName) {
                        VeinComponent::ComponentData *currentContentSetsCData = new VeinComponent::ComponentData();
                        currentContentSetsCData->setEntityId(m_dPtr->m_entityId);
                        currentContentSetsCData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
                        currentContentSetsCData->setComponentName(DataLoggerPrivate::s_currentContentSetsComponentName);
                        currentContentSetsCData->setNewValue(cData->newValue());
                        currentContentSetsCData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
                        currentContentSetsCData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

                        emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, currentContentSetsCData));
                    }
                    else if(cData->componentName() == DataLoggerPrivate::s_availableContentSetsComponentName) {
                        VeinComponent::ComponentData *availableContentSetsCData = new VeinComponent::ComponentData();
                        availableContentSetsCData->setEntityId(m_dPtr->m_entityId);
                        availableContentSetsCData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
                        availableContentSetsCData->setComponentName(DataLoggerPrivate::s_availableContentSetsComponentName);
                        availableContentSetsCData->setNewValue(cData->newValue());
                        availableContentSetsCData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
                        availableContentSetsCData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

                        emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, availableContentSetsCData));
                    }

                    t_event->accept();
                }
            }
        }
        else if(evData->type()==VeinComponent::RemoteProcedureData::dataType() &&
                evData->entityId() == m_dPtr->m_entityId) {
            VeinComponent::RemoteProcedureData *rpcData=nullptr;
            rpcData = static_cast<VeinComponent::RemoteProcedureData *>(cEvent->eventData());
            if(rpcData->command() == VeinComponent::RemoteProcedureData::Command::RPCMD_CALL){
                if(m_dPtr->m_rpcList.contains(rpcData->procedureName())){
                    const QUuid callId = rpcData->invokationData().value(VeinComponent::RemoteProcedureData::s_callIdString).toUuid();
                    m_dPtr->m_rpcList[rpcData->procedureName()]->callFunction(callId,cEvent->peerId(),rpcData->invokationData());
                    cEvent->accept();
                }else if(!cEvent->isAccepted()){
                    qWarning() << "No remote procedure with entityId:" << m_dPtr->m_entityId << "name:" << rpcData->procedureName();
                    VF_ASSERT(false, QStringC(QString("No remote procedure with entityId: %1 name: %2").arg(m_dPtr->m_entityId).arg(rpcData->procedureName())));
                    VeinComponent::ErrorData *eData = new VeinComponent::ErrorData();
                    eData->setEntityId(m_dPtr->m_entityId);
                    eData->setErrorDescription(QString("No remote procedure with name: %1").arg(rpcData->procedureName()));
                    eData->setOriginalData(rpcData);
                    eData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
                    eData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);
                    VeinEvent::CommandEvent *errorEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, eData);
                    errorEvent->setPeerId(cEvent->peerId());
                    cEvent->accept();
                    emit sigSendEvent(errorEvent);
                }
            }
        }
    }
}
}
