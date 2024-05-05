#include "dataloggerprivate.h"
#include "loggerstatictexts.h"
#include <vcmp_entitydata.h>
#include <vf_server_component_setter.h>
#include <QFileInfo>

using namespace VeinLogger;

DataLoggerPrivate::DataLoggerPrivate(DatabaseLogger *qPtr) : m_qPtr(qPtr)
{
}

DataLoggerPrivate::~DataLoggerPrivate()
{
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
        componentData.insert(LoggerStaticTexts::s_entityNameComponentName, m_qPtr->entityName());
        componentData.insert(LoggerStaticTexts::s_loggingEnabledComponentName, QVariant(false));
        componentData.insert(LoggerStaticTexts::s_loggingStatusTextComponentName, QVariant(QString("No database selected")));
        ///@todo load from persistent settings file?
        componentData.insert(LoggerStaticTexts::s_databaseReadyComponentName, QVariant(false));
        componentData.insert(LoggerStaticTexts::s_databaseFileComponentName, QVariant(QString()));
        componentData.insert(LoggerStaticTexts::s_scheduledLoggingEnabledComponentName, QVariant(false));
        componentData.insert(LoggerStaticTexts::s_scheduledLoggingDurationComponentName, QVariant());
        componentData.insert(LoggerStaticTexts::s_scheduledLoggingCountdownComponentName, QVariant(0));
        componentData.insert(LoggerStaticTexts::s_existingSessionsComponentName, QStringList());
        componentData.insert(LoggerStaticTexts::s_customerDataComponentName, QString());
        componentData.insert(LoggerStaticTexts::loggedComponentsComponentName, QVariantMap());

        // TODO: Add more from modulemanager
        componentData.insert(LoggerStaticTexts::s_sessionNameComponentName, QString());
        componentData.insert(LoggerStaticTexts::s_guiContextComponentName, QString());
        componentData.insert(LoggerStaticTexts::s_transactionNameComponentName, QString());
        componentData.insert(LoggerStaticTexts::s_currentContentSetsComponentName, QVariantList());
        componentData.insert(LoggerStaticTexts::s_availableContentSetsComponentName, QVariantList());

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

        m_initDone = true;
    }
}
