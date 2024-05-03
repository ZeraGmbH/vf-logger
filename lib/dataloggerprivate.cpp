#include "dataloggerprivate.h"
#include <vcmp_entitydata.h>
#include <vf_server_component_setter.h>
#include <QFileInfo>

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
const QLatin1String DataLoggerPrivate::loggedComponentsComponentName = QLatin1String("LoggedComponents");

using namespace VeinLogger;

DataLoggerPrivate::DataLoggerPrivate(DatabaseLogger *qPtr) : m_qPtr(qPtr)
{
    m_batchedExecutionTimer.setInterval(5000);
    m_batchedExecutionTimer.setSingleShot(false);
}

DataLoggerPrivate::~DataLoggerPrivate()
{
    m_batchedExecutionTimer.stop();
    m_asyncDatabaseThread.quit();
    m_asyncDatabaseThread.wait();
}

void DataLoggerPrivate::initOnce()
{
    Q_ASSERT(m_initDone == false);
    if(m_initDone == false) {
        VeinComponent::EntityData *systemData = new VeinComponent::EntityData();
        systemData->setCommand(VeinComponent::EntityData::Command::ECMD_ADD);
        systemData->setEntityId(m_qPtr->entityId());

        VeinEvent::CommandEvent *systemEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, systemData);

        emit m_qPtr->sigSendEvent(systemEvent);

        VeinComponent::ComponentData *initialData = nullptr;

        QHash<QString, QVariant> componentData;
        componentData.insert(s_entityNameComponentName, m_entityName);
        componentData.insert(s_loggingEnabledComponentName, QVariant(false));
        componentData.insert(s_loggingStatusTextComponentName, QVariant(QString("No database selected")));
        ///@todo load from persistent settings file?
        componentData.insert(s_databaseReadyComponentName, QVariant(false));
        componentData.insert(s_databaseFileComponentName, QVariant(QString()));
        componentData.insert(s_scheduledLoggingEnabledComponentName, QVariant(false));
        componentData.insert(s_scheduledLoggingDurationComponentName, QVariant());
        componentData.insert(s_scheduledLoggingCountdownComponentName, QVariant(0));
        componentData.insert(s_existingSessionsComponentName, QStringList());
        componentData.insert(s_customerDataComponentName, QString());
        componentData.insert(loggedComponentsComponentName, QVariantMap());

        // TODO: Add more from modulemanager
        componentData.insert(s_sessionNameComponentName, QString());
        componentData.insert(s_guiContextComponentName, QString());
        componentData.insert(s_transactionNameComponentName, QString());
        componentData.insert(s_currentContentSetsComponentName, QVariantList());
        componentData.insert(s_availableContentSetsComponentName, QVariantList());

        for(const QString &componentName : componentData.keys()) {
            initialData = new VeinComponent::ComponentData();
            initialData->setEntityId(m_qPtr->entityId());
            initialData->setCommand(VeinComponent::ComponentData::Command::CCMD_ADD);
            initialData->setComponentName(componentName);
            initialData->setNewValue(componentData.value(componentName));
            initialData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
            initialData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

            systemEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, initialData);
            emit m_qPtr->sigSendEvent(systemEvent);
        }

        VfCpp::cVeinModuleRpc::Ptr tmpval;
        tmpval= VfCpp::cVeinModuleRpc::Ptr(new VfCpp::cVeinModuleRpc(m_qPtr->entityId(),m_qPtr,m_qPtr,"RPC_deleteSession",VfCpp::cVeinModuleRpc::Param({{"p_session", "QString"}})), &QObject::deleteLater);
        m_rpcList[tmpval->rpcName()]=tmpval;

        initStateMachine();
        m_initDone = true;
    }
}

void DataLoggerPrivate::initStateMachine()
{
    m_parallelWrapperState->setChildMode(QStateMachine::ParallelStates);
    m_stateMachine.setInitialState(m_parallelWrapperState);
    m_loggingContainerState->setInitialState(m_loggingDisabledState);
    m_logSchedulerContainerState->setInitialState(m_logSchedulerDisabledState);

    //enabled -> disabled
    m_loggingEnabledState->addTransition(m_qPtr, &DatabaseLogger::sigLoggingStopped, m_loggingDisabledState);
    //disabled -> enabled
    m_loggingDisabledState->addTransition(m_qPtr, &DatabaseLogger::sigLoggingStarted, m_loggingEnabledState);

    //enabled -> disbled
    m_logSchedulerEnabledState->addTransition(m_qPtr, &DatabaseLogger::sigLogSchedulerDeactivated, m_logSchedulerDisabledState);
    //disabled -> enabled
    m_logSchedulerDisabledState->addTransition(m_qPtr, &DatabaseLogger::sigLogSchedulerActivated, m_logSchedulerEnabledState);

    QObject::connect(m_loggingDisabledState, &QState::entered, this, [&](){
        m_batchedExecutionTimer.stop();
    });
    QObject::connect(m_logSchedulerEnabledState, &QState::entered, [&](){
        VeinComponent::ComponentData *schedulingEnabledCData = new VeinComponent::ComponentData();
        schedulingEnabledCData->setEntityId(m_qPtr->entityId());
        schedulingEnabledCData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
        schedulingEnabledCData->setComponentName(DataLoggerPrivate::s_scheduledLoggingEnabledComponentName);
        schedulingEnabledCData->setNewValue(true);
        schedulingEnabledCData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
        schedulingEnabledCData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

        emit m_qPtr->sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, schedulingEnabledCData));
    });
    QObject::connect(m_logSchedulerDisabledState, &QState::entered, [&](){
        VeinComponent::ComponentData *schedulingDisabledCData = new VeinComponent::ComponentData();
        schedulingDisabledCData->setEntityId(m_qPtr->entityId());
        schedulingDisabledCData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
        schedulingDisabledCData->setComponentName(DataLoggerPrivate::s_scheduledLoggingEnabledComponentName);
        schedulingDisabledCData->setNewValue(false);
        schedulingDisabledCData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
        schedulingDisabledCData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

        emit m_qPtr->sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, schedulingDisabledCData));
    });

    m_stateMachine.start();
}

void DataLoggerPrivate::updateSchedulerCountdown()
{
    if(m_schedulingTimer.isActive()) {
        VeinComponent::ComponentData *schedulerCountdownCData = new VeinComponent::ComponentData();
        schedulerCountdownCData->setEntityId(m_qPtr->entityId());
        schedulerCountdownCData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
        schedulerCountdownCData->setComponentName(DataLoggerPrivate::s_scheduledLoggingCountdownComponentName);
        schedulerCountdownCData->setNewValue(QVariant(m_schedulingTimer.remainingTime()));
        schedulerCountdownCData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
        schedulerCountdownCData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

        emit m_qPtr->sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, schedulerCountdownCData));
    }
}
