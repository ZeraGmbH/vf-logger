#include "dataloggerprivate.h"
#include <vcmp_entitydata.h>
#include <QFileInfo>
#include <QStorageInfo>

const QLatin1String DataLoggerPrivate::s_entityNameComponentName = QLatin1String("EntityName");
const QLatin1String DataLoggerPrivate::s_loggingStatusTextComponentName  = QLatin1String("LoggingStatus");
const QLatin1String DataLoggerPrivate::s_loggingEnabledComponentName = QLatin1String("LoggingEnabled");
const QLatin1String DataLoggerPrivate::s_databaseReadyComponentName = QLatin1String("DatabaseReady");
const QLatin1String DataLoggerPrivate::s_databaseFileComponentName = QLatin1String("DatabaseFile");
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

void DataLoggerPrivate::initOnce()
{
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
        componentData.insert(s_scheduledLoggingEnabledComponentName, QVariant(false));
        componentData.insert(s_scheduledLoggingDurationComponentName, QVariant());
        componentData.insert(s_scheduledLoggingCountdownComponentName, QVariant(0));
        componentData.insert(s_existingSessionsComponentName, QStringList());
        componentData.insert(s_customerDataComponentName, QString());

        // TODO: Add more from modulemanager
        componentData.insert(s_sessionNameComponentName, QString());
        componentData.insert(s_guiContextComponentName, QString());
        componentData.insert(s_transactionNameComponentName, QString());
        componentData.insert(s_currentContentSetsComponentName, QVariantList());
        componentData.insert(s_availableContentSetsComponentName, QVariantList());

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

void DataLoggerPrivate::setStatusText(const QString &t_status)
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


void DataLoggerPrivate::initStateMachine()
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
            if(!tmpDir.isRoot())
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

void DataLoggerPrivate::updateSchedulerCountdown()
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
