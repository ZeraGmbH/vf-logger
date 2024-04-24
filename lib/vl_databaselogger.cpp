#include "vl_databaselogger.h"
#include "vl_globallabels.h"
#include "dataloggerprivate.h"
#include "loggercontentsetconfig.h"
#include <vl_componentunion.h>
#include <vcmp_entitydata.h>
#include <vcmp_errordata.h>
#include <vf-cpp-rpc.h>
#include <vf_client_component_setter.h>
#include <vf_server_component_setter.h>
#include <QHash>
#include <QDir>
#include <QJsonDocument>

namespace VeinLogger
{
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
        m_entityId = 2;
        m_dPtr->m_entityName = QLatin1String("_LoggingSystem");
        qInfo() << "Created plaintext logger:" << m_dPtr->m_entityName << "with id:" << m_entityId;
        break;
    }
    case AbstractLoggerDB::STORAGE_MODE::BINARY: {
        //use different id and entity name
        m_entityId = 200000;
        m_dPtr->m_entityName = QLatin1String("_BinaryLoggingSystem");
        qInfo() <<  "Created binary logger:" << m_dPtr->m_entityName << "with id:" << m_entityId;
        break;
    }
    }

    connect(this, &DatabaseLogger::sigAttached, [this](){ m_dPtr->initOnce(); });
    connect(&m_dPtr->m_batchedExecutionTimer, &QTimer::timeout, [this]() {
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

    // db error handling
    connect(this, &DatabaseLogger::sigDatabaseError, [this](const QString &t_errorString) {
        qWarning() << t_errorString;
        closeDatabase();
        m_dPtr->m_noUninitMessage = true;
        m_dPtr->setStatusText("Database error");
    });
}

DatabaseLogger::~DatabaseLogger()
{
    delete m_dPtr;
}

void DatabaseLogger::prepareLogging()
{
    const QSet<QAbstractState*> requiredStates = {m_dPtr->m_loggingEnabledState, m_dPtr->m_databaseReadyState};
    if(m_dPtr->m_stateMachine.configuration().contains(requiredStates)) {
        QString tmpContentSets = m_contentSets.join(QLatin1Char(','));
        m_transactionId = m_dPtr->m_database->addTransaction(m_transactionName, m_dbSessionName, tmpContentSets, m_guiContext);

        m_dPtr->m_database->addStartTime(m_transactionId, QDateTime::currentDateTime());

        // add stored values at start
        for(const int tmpEntityId : m_loggedValues.uniqueKeys()) {
            const QList<QString> tmpComponents = m_loggedValues.values(tmpEntityId);
            for(const QString &tmpComponentName : tmpComponents) {
                if(m_dPtr->m_dataSource->hasEntity(tmpEntityId)) { // is entity in storage?
                    QStringList componentNamesToAdd;
                    if(tmpComponentName == VLGlobalLabels::allComponentsName())
                        componentNamesToAdd = m_dPtr->m_dataSource->getEntityComponentsForStore(tmpEntityId);
                    else
                        componentNamesToAdd.append(tmpComponentName);

                    for (auto componentToAdd : componentNamesToAdd) {
                        const QVariant storedValue = m_dPtr->m_dataSource->getValue(tmpEntityId, componentToAdd);
                        addValueToDb(storedValue, tmpEntityId, componentToAdd);
                    }
                }
            }
        }
    }
}

bool DatabaseLogger::loggingEnabled() const
{
    return m_dPtr->m_stateMachine.configuration().contains(m_dPtr->m_loggingEnabledState);
}

int DatabaseLogger::entityId() const
{
    return m_entityId;
}

QString DatabaseLogger::entityName() const
{
    return m_dPtr->m_entityName;
}

QString DatabaseLogger::getTransactionName() const
{
    return m_transactionName;
}

QString DatabaseLogger::getDbSessionName() const
{
    return m_dbSessionName;
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

        QEvent *event = VfServerComponentSetter::generateEvent(m_entityId, DataLoggerPrivate::s_loggingEnabledComponentName,
                                                               QVariant(), t_enabled);
        emit sigSendEvent(event);
    }
}

bool DatabaseLogger::openDatabase(const QString &t_filePath)
{
    m_dPtr->m_databaseFilePath = t_filePath;
    m_dPtr->m_noUninitMessage = false;

    QEvent *event = VfServerComponentSetter::generateEvent(m_entityId, DataLoggerPrivate::s_databaseFileComponentName,
                                                            QVariant(), t_filePath);
    emit sigSendEvent(event);

    const bool validStorage = m_dPtr->checkDBFilePath(t_filePath); // throws sigDatabaseError on error
    if(validStorage) {
        if(m_dPtr->m_database != nullptr) {
            disconnect(m_dPtr->m_database, &AbstractLoggerDB::sigDatabaseError, this, &DatabaseLogger::sigDatabaseError);
            m_dPtr->m_database->deleteLater();
            m_dPtr->m_database = nullptr;
        }
        m_dPtr->m_asyncDatabaseThread.quit();
        m_dPtr->m_asyncDatabaseThread.wait();
        m_dPtr->m_database = m_dPtr->m_databaseFactory();//new SQLiteDB(t_storageMode);
        // forward database's error my handler
        connect(m_dPtr->m_database, &AbstractLoggerDB::sigDatabaseError, this, &DatabaseLogger::sigDatabaseError);
        m_dPtr->m_database->setStorageMode(m_dPtr->m_storageMode);
        if(m_dPtr->m_database->requiresOwnThread()) {
            m_dPtr->m_database->moveToThread(&m_dPtr->m_asyncDatabaseThread);
            m_dPtr->m_asyncDatabaseThread.start();
        }

        // connections to m_dPtr->m_database are queued for requiresOwnThread() == true
        connect(this, &DatabaseLogger::sigAddLoggedValue, m_dPtr->m_database, &AbstractLoggerDB::addLoggedValue);
        connect(this, &DatabaseLogger::sigAddEntity, m_dPtr->m_database, &AbstractLoggerDB::addEntity);
        connect(this, &DatabaseLogger::sigAddComponent, m_dPtr->m_database, &AbstractLoggerDB::addComponent);
        connect(this, &DatabaseLogger::sigAddSession, m_dPtr->m_database, &AbstractLoggerDB::addSession);
        connect(this, &DatabaseLogger::sigOpenDatabase, m_dPtr->m_database, &AbstractLoggerDB::openDatabase);
        connect(m_dPtr->m_database, &AbstractLoggerDB::sigDatabaseReady, this, &DatabaseLogger::sigDatabaseReady);
        connect(m_dPtr->m_database, &AbstractLoggerDB::sigNewSessionList, this, &DatabaseLogger::updateSessionList);

        connect(&m_dPtr->m_batchedExecutionTimer, &QTimer::timeout, m_dPtr->m_database, &AbstractLoggerDB::runBatchedExecution);
        // run final batch instantly when logging is disabled
        connect(m_dPtr->m_loggingDisabledState, &QAbstractState::entered, m_dPtr->m_database, &AbstractLoggerDB::runBatchedExecution);

        emit sigOpenDatabase(t_filePath);
    }
    return validStorage;
}

void DatabaseLogger::closeDatabase()
{
    m_dPtr->m_noUninitMessage = false;
    setLoggingEnabled(false);
    if(m_dPtr->m_database != nullptr) {
        disconnect(m_dPtr->m_database, &AbstractLoggerDB::sigDatabaseError, this, &DatabaseLogger::sigDatabaseError);
        m_dPtr->m_database->deleteLater();
        m_dPtr->m_database = nullptr;
    }
    m_dPtr->m_asyncDatabaseThread.quit();
    m_dPtr->m_asyncDatabaseThread.wait();
    if(m_dPtr->m_deleteWatcher.directories().count()) {
        const QStringList watchedDirs = m_dPtr->m_deleteWatcher.directories();
        for(const QString &watchDir : watchedDirs)
            m_dPtr->m_deleteWatcher.removePath(watchDir);
        QObject::disconnect(&m_dPtr->m_deleteWatcher, &QFileSystemWatcher::directoryChanged, this, &DatabaseLogger::checkDatabaseStillValid);
    }
    emit sigDatabaseUnloaded();

    QString closedDb = m_dPtr->m_databaseFilePath;
    m_dPtr->m_databaseFilePath.clear();

    // set vein database file name empty
    QEvent* event = VfServerComponentSetter::generateEvent(m_entityId, DataLoggerPrivate::s_databaseFileComponentName,
                                                           QVariant(), QString());
    emit sigSendEvent(event);
    // set CustomerData component empty
    event = VfServerComponentSetter::generateEvent(m_entityId, DataLoggerPrivate::s_customerDataComponentName,
                                                   QVariant(), QString());
    emit sigSendEvent(event);

    updateSessionList(QStringList());

    qInfo() << "Unloaded database:" << closedDb;
}

void DatabaseLogger::checkDatabaseStillValid()
{
    QFile dbFile(m_dPtr->m_databaseFilePath);
    if(!dbFile.exists())
        emit sigDatabaseError(QString("Watcher detected database file %1 is gone!").arg(m_dPtr->m_databaseFilePath));
}

void DatabaseLogger::updateSessionList(QStringList sessionNames)
{
    QEvent* event = VfServerComponentSetter::generateEvent(m_entityId, DataLoggerPrivate::s_existingSessionsComponentName,
                                                           QVariant(), sessionNames);
    emit sigSendEvent(event);
}

void DatabaseLogger::onModmanSessionChange(QVariant newSession)
{
    for (auto &env : LoggerContentSetConfig::getConfigEnvironment())
        env.m_loggerContentHandler->setSession(newSession.toString());
    const QStringList availContentSetStrings = LoggerContentSetConfig::getAvailableContentSets();

    // Avoid QVariant type cast - is there a more simple way?
    QVariantList availContentSets;
    for(const QString &availContentSetString : availContentSetStrings)
        availContentSets.append(availContentSetString);

    QEvent* event = VfServerComponentSetter::generateEvent(m_entityId, DataLoggerPrivate::s_availableContentSetsComponentName,
                                                           QVariant(), availContentSets);
    emit sigSendEvent(event);
}

QVariant DatabaseLogger::RPC_deleteSession(QVariantMap p_parameters)
{
    QString session = p_parameters["p_session"].toString();
    QVariant retVal = m_dPtr->m_database->deleteSession(session);
    // check if deleted session is current Session and if it is set sessionName empty
    // We will not check retVal here. If something goes wrong and the session is still available the
    // user can choose it again without risking undefined behavior.
    if(session == m_dbSessionName) {
        VeinComponent::ComponentData *sessionNameCData = new VeinComponent::ComponentData();
        sessionNameCData ->setEntityId(m_entityId);
        sessionNameCData ->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
        sessionNameCData ->setComponentName(DataLoggerPrivate::s_sessionNameComponentName);
        sessionNameCData ->setNewValue(QString());
        sessionNameCData ->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
        sessionNameCData ->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);
        m_dbSessionName = "";
        emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, sessionNameCData));
    }
    return retVal;
}

void DatabaseLogger::handleLoggedComponentsTransaction(VeinComponent::ComponentData *cData)
{
    QVariant oldValue = cData->oldValue();
    QVariant newValue = cData->newValue();
    if(oldValue != newValue)
        handleLoggedComponentsChange(newValue);

    QEvent* event = VfServerComponentSetter::generateEvent(m_entityId, cData->componentName(), oldValue, newValue);
    emit sigSendEvent(event);
}

void DatabaseLogger::handleLoggedComponentsChange(QVariant newValue)
{
    if(static_cast<QMetaType::Type>(newValue.type()) != QMetaType::QVariantMap) {
        qWarning("DatabaseLogger::handleLoggedComponentsChange: wrong type");
        return;
    }
    clearLoggerEntries();

    QVariantMap entityComponentMap = newValue.toMap();
    QStringList entityIdsStr = entityComponentMap.keys();
    for(const auto &entityIdStr : entityIdsStr) {
        QVariantList componentList = entityComponentMap[entityIdStr].toList();
        if(componentList.size()) {
            for(auto &component : qAsConst(componentList))
                addLoggerEntry(entityIdStr.toInt(), component.toString());
        }
        else {
            // We need to add a special component name to inform logger
            // to store all components
            addLoggerEntry(entityIdStr.toInt(), VLGlobalLabels::allComponentsName());
        }
    }
}

bool DatabaseLogger::isLoggedComponent(int entityId, const QString &componentName) const
{
    bool storeComponent = false;
    if(m_loggedValues.contains(entityId, VLGlobalLabels::allComponentsName())) {
        QStringList componentsNoStore = VLGlobalLabels::noStoreComponents();
        storeComponent = !componentsNoStore.contains(componentName);
    }
    else
        storeComponent = m_loggedValues.contains(entityId, componentName);
    return storeComponent;

}

void DatabaseLogger::addLoggerEntry(int t_entityId, const QString &t_componentName)
{
    if(m_loggedValues.contains(t_entityId, t_componentName) == false)
        m_loggedValues.insert(t_entityId, t_componentName);
}

void DatabaseLogger::clearLoggerEntries()
{
    m_loggedValues.clear();
}

QVariant DatabaseLogger::handleVeinDbSessionNameSet(QString sessionName)
{
    QVariant sessionCustomerDataName;
    if(!m_dPtr->m_database->hasSessionName(sessionName)) {
        QMultiHash<int, QString> tmpStaticComps;
        // Add customer data once per session
        if(m_dPtr->m_dataSource->hasEntity(200))
            for(const QString &comp : m_dPtr->m_dataSource->getEntityComponentsForStore(200))
                tmpStaticComps.insert(200, comp);
        // Add status module once per session
        if(m_dPtr->m_dataSource->hasEntity(1150))
            for(const QString &comp : m_dPtr->m_dataSource->getEntityComponentsForStore(1150))
                tmpStaticComps.insert(1150, comp);

        QList<QVariantMap> tmpStaticData;
        for(const int tmpEntityId : tmpStaticComps.uniqueKeys()) { //only process once for every entity
            if(!m_dPtr->m_database->hasEntityId(tmpEntityId))
                emit sigAddEntity(tmpEntityId, m_dPtr->m_dataSource->getEntityName(tmpEntityId));
            const QList<QString> tmpComponents = tmpStaticComps.values(tmpEntityId);
            for(const QString &tmpComponentName : tmpComponents) {
                if(!m_dPtr->m_database->hasComponentName(tmpComponentName))
                    emit sigAddComponent(tmpComponentName);
                QVariantMap tmpMap;
                tmpMap["entityId"] = tmpEntityId;
                tmpMap["compName"] = tmpComponentName;
                tmpMap["value"] = m_dPtr->m_dataSource->getValue(tmpEntityId, tmpComponentName);
                tmpMap["time"] = QDateTime::currentDateTime();
                tmpStaticData.append(tmpMap);
            }
        }

        // We are reading it like this because it is faster than writing it to the db and then reading it agian
        sessionCustomerDataName = m_dPtr->m_dataSource->getValue(200, "FileSelected");
        emit sigAddSession(sessionName, tmpStaticData);
    }
    else
        sessionCustomerDataName = m_dPtr->m_database->readSessionComponent(sessionName,"CustomerData", "FileSelected").toString();
    return sessionCustomerDataName;
}

void DatabaseLogger::tryInitModmanSessionComponent()
{
    if(!m_modmanSessionComponent) {
        VeinEvent::StorageSystem *storage = m_dPtr->m_dataSource->getStorageSystem();
        m_modmanSessionComponent = storage->getComponent(0, "Session");
        if(m_modmanSessionComponent && m_modmanSessionComponent->getValue().isValid()) {
            onModmanSessionChange(m_modmanSessionComponent->getValue());
            connect(m_modmanSessionComponent.get(), &VeinEvent::StorageComponentInterface::sigValueChange,
                    this, &DatabaseLogger::onModmanSessionChange);
        }
    }
}

void DatabaseLogger::addValueToDb(const QVariant newValue, const int entityId, const QString componentName)
{
    QString entityName = m_dPtr->m_dataSource->getEntityName(entityId);
    if(!m_dPtr->m_database->hasEntityId(entityId))
        emit sigAddEntity(entityId, entityName);
    if(!m_dPtr->m_database->hasComponentName(componentName))
        emit sigAddComponent(componentName);
    if(isLoggedComponent(entityId, componentName))
        emit sigAddLoggedValue(m_dbSessionName, QVector<int>() << m_transactionId, entityId, componentName, newValue, QDateTime::currentDateTime());
}

void DatabaseLogger::processEvent(QEvent *t_event)
{
    using namespace VeinEvent;
    using namespace VeinComponent;
    if(t_event->type()==CommandEvent::getQEventType()) {
        CommandEvent *cEvent = static_cast<CommandEvent *>(t_event);
        EventData *evData = cEvent->eventData();

        const QSet<QAbstractState*> activeStates = m_dPtr->m_stateMachine.configuration();
        const QSet<QAbstractState*> requiredStates = {m_dPtr->m_loggingEnabledState, m_dPtr->m_databaseReadyState};
        if(evData->type() == ComponentData::dataType()) {

            ComponentData *cData = static_cast<ComponentData *>(evData);
            const int entityId = evData->entityId();
            const QString componentName = cData->componentName();
            const QVariant newValue = cData->newValue();

            if(cEvent->eventSubtype() == CommandEvent::EventSubtype::NOTIFICATION) {
                tryInitModmanSessionComponent();

                ///@todo check if the setLoggingEnabled() call can be moved to the transaction code block for s_loggingEnabledComponentName
                if(entityId == m_entityId && componentName == DataLoggerPrivate::s_loggingEnabledComponentName) {
                    bool loggingEnabled = newValue.toBool();
                    if(loggingEnabled) {
                        bool validConditions = true;
                        if(getDbSessionName().isEmpty()) {
                            validConditions = false;
                            qWarning("Logging requires a valid sessionName!");
                        }
                        if(getTransactionName().isEmpty()) {
                            validConditions = false;
                            qWarning("Logging requires a valid transactionName!");
                        }
                        if(validConditions) {
                            prepareLogging();
                            setLoggingEnabled(loggingEnabled);
                        }
                    }
                    else
                        setLoggingEnabled(loggingEnabled);
                }

                if(activeStates.contains(requiredStates) && !m_dbSessionName.isEmpty())
                    addValueToDb(newValue, entityId, componentName);
            }

            else if(cEvent->eventSubtype() == CommandEvent::EventSubtype::TRANSACTION &&
                    entityId == m_entityId) {
                Q_ASSERT(cData != nullptr);

                if(cData->eventCommand() == VeinComponent::ComponentData::Command::CCMD_SET) {
                    if(componentName == DataLoggerPrivate::loggedComponentsComponentName)
                        handleLoggedComponentsTransaction(cData);
                    else if(componentName == DataLoggerPrivate::s_databaseFileComponentName) {
                        if(m_dPtr->m_database == nullptr || newValue != m_dPtr->m_databaseFilePath) {
                            if(newValue.toString().isEmpty()) //unsetting the file component = closing the database
                                closeDatabase();
                            else
                                openDatabase(newValue.toString());
                        }

                        // a good place to reset selected sessionName - however db-open ends up with
                        QEvent* event = VfServerComponentSetter::generateEvent(m_entityId, DataLoggerPrivate::s_sessionNameComponentName,
                                                                               cData->oldValue(), QString());
                        emit sigSendEvent(event);
                        m_dbSessionName = "";
                    }
                    else if(componentName == DataLoggerPrivate::s_loggingEnabledComponentName) {
                        QEvent* event = VfServerComponentSetter::generateEvent(m_entityId, DataLoggerPrivate::s_loggingEnabledComponentName,
                                                                               cData->oldValue(), newValue);
                        emit sigSendEvent(event);
                    }
                    else if(componentName == DataLoggerPrivate::s_scheduledLoggingEnabledComponentName) {
                        //do not accept values that are already set
                        if(newValue.toBool() != m_dPtr->m_stateMachine.configuration().contains(m_dPtr->m_logSchedulerEnabledState)) {
                            if(newValue.toBool() == true)
                                emit sigLogSchedulerActivated();
                            else
                                emit sigLogSchedulerDeactivated();
                            setLoggingEnabled(false);
                        }
                    }
                    else if(componentName == DataLoggerPrivate::s_scheduledLoggingDurationComponentName) {
                        bool invalidTime = false;
                        bool conversionOk = false;
                        const int logDurationMsecs = newValue.toInt(&conversionOk);
                        invalidTime = !conversionOk;

                        if(conversionOk == true && logDurationMsecs != m_dPtr->m_scheduledLoggingDuration) {
                            m_dPtr->m_scheduledLoggingDuration = logDurationMsecs;
                            if(logDurationMsecs > 0) {
                                m_dPtr->m_schedulingTimer.setInterval(logDurationMsecs);
                                if(activeStates.contains(requiredStates))
                                    m_dPtr->m_schedulingTimer.start();

                                VeinComponent::ComponentData *schedulingDurationData = new VeinComponent::ComponentData();
                                schedulingDurationData->setEntityId(m_entityId);
                                schedulingDurationData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
                                schedulingDurationData->setComponentName(DataLoggerPrivate::s_scheduledLoggingDurationComponentName);
                                schedulingDurationData->setNewValue(newValue);
                                schedulingDurationData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
                                schedulingDurationData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

                                emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, schedulingDurationData));
                            }
                            else
                                invalidTime = true;
                        }
                        if(invalidTime) {
                            VeinComponent::ErrorData *errData = new VeinComponent::ErrorData();
                            errData->setEntityId(m_entityId);
                            errData->setOriginalData(cData);
                            errData->setEventOrigin(VeinComponent::ErrorData::EventOrigin::EO_LOCAL);
                            errData->setEventTarget(VeinComponent::ErrorData::EventTarget::ET_ALL);
                            errData->setErrorDescription(QString("Invalid logging duration: %1").arg(newValue.toString()));

                            emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, errData));
                        }
                    }
                    else if(componentName == DataLoggerPrivate::s_sessionNameComponentName) {
                        m_dbSessionName = newValue.toString();

                        QString sessionName = newValue.toString();
                        // we have a working database?
                        QVariant sessionCustomerDataName;
                        if(!sessionName.isEmpty() &&
                           m_dPtr->m_database &&
                           m_dPtr->m_database->databaseIsOpen()) {
                            sessionCustomerDataName = handleVeinDbSessionNameSet(sessionName);
                        }

                        QEvent *event;
                        event = VfServerComponentSetter::generateEvent(m_entityId, DataLoggerPrivate::s_sessionNameComponentName,
                                                                       cData->oldValue(), newValue);
                        emit sigSendEvent(event);
                        event = VfServerComponentSetter::generateEvent(m_entityId, DataLoggerPrivate::s_customerDataComponentName,
                                                                       QVariant(), sessionCustomerDataName);
                        emit sigSendEvent(event);
                    }
                    else if(componentName == DataLoggerPrivate::s_guiContextComponentName) {
                        m_guiContext = newValue.toString();
                        QEvent *event = VfServerComponentSetter::generateEvent(m_entityId, DataLoggerPrivate::s_guiContextComponentName,
                                                                       cData->oldValue(), newValue);
                        emit sigSendEvent(event);
                    }
                    else if(componentName == DataLoggerPrivate::s_transactionNameComponentName) {
                        m_transactionName = newValue.toString();
                        QEvent* event = VfServerComponentSetter::generateEvent(m_entityId, DataLoggerPrivate::s_transactionNameComponentName,
                                                                               cData->oldValue(), newValue);
                        emit sigSendEvent(event);
                    }
                    else if(componentName == DataLoggerPrivate::s_currentContentSetsComponentName) {
                        m_contentSets = newValue.toStringList();
                        QVariantMap loggedComponents = readContentSets();
                        clearLoggerEntries();

                        QEvent* event;
                        // Client: we choose same as a client would - it can still access availableContentSets
                        event = VfClientComponentSetter::generateEvent(m_entityId, DataLoggerPrivate::loggedComponentsComponentName,
                                                                       QVariant(), loggedComponents);
                        emit sigSendEvent(event);

                        event = VfServerComponentSetter::generateEvent(m_entityId, DataLoggerPrivate::s_currentContentSetsComponentName,
                                                                       cData->oldValue(), newValue);
                        emit sigSendEvent(event);
                    }

                    t_event->accept();
                }
            }
        }
        else if(evData->type()==VeinComponent::RemoteProcedureData::dataType() &&
                evData->entityId() == m_entityId) {
            VeinComponent::RemoteProcedureData *rpcData=nullptr;
            rpcData = static_cast<VeinComponent::RemoteProcedureData *>(cEvent->eventData());
            if(rpcData->command() == VeinComponent::RemoteProcedureData::Command::RPCMD_CALL){
                if(m_dPtr->m_rpcList.contains(rpcData->procedureName())){
                    const QUuid callId = rpcData->invokationData().value(VeinComponent::RemoteProcedureData::s_callIdString).toUuid();
                    m_dPtr->m_rpcList[rpcData->procedureName()]->callFunction(callId,cEvent->peerId(),rpcData->invokationData());
                    cEvent->accept();
                }
                else if(!cEvent->isAccepted()) {
                    qWarning() << "No remote procedure with entityId:" << m_entityId << "name:" << rpcData->procedureName();
                    VF_ASSERT(false, QStringC(QString("No remote procedure with entityId: %1 name: %2").arg(m_entityId).arg(rpcData->procedureName())));
                    VeinComponent::ErrorData *eData = new VeinComponent::ErrorData();
                    eData->setEntityId(m_entityId);
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

QVariantMap DatabaseLogger::readContentSets()
{
    typedef QMap<int, QStringList> TEcMap;
    TEcMap tmpResultMap;
    for(auto &contentSet : m_contentSets) {
        for(auto &confEnv: LoggerContentSetConfig::getConfigEnvironment()) {
            TEcMap ecMap = confEnv.m_loggerContentHandler->getEntityComponents(contentSet);
            for(TEcMap::const_iterator iter=ecMap.constBegin(); iter!=ecMap.constEnd(); ++iter)
                ComponentUnion::uniteComponents(tmpResultMap, iter.key(), iter.value());
        }
    }
    QVariantMap resultMap;
    TEcMap::const_iterator iter;
    for(iter=tmpResultMap.constBegin(); iter!=tmpResultMap.constEnd(); ++iter) {
        resultMap[QString::number(iter.key())] = tmpResultMap[iter.key()];
    }
    return resultMap;
}

}
