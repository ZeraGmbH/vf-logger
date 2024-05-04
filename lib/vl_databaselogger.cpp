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
#include <QStorageInfo>
#include <QJsonDocument>

namespace VeinLogger
{
DatabaseLogger::DatabaseLogger(VeinEvent::StorageSystem *veinStorage, DBFactory factoryFunction, QObject *parent, AbstractLoggerDB::STORAGE_MODE storageMode) :
    VeinEvent::EventSystem(parent),
    m_dPtr(new DataLoggerPrivate(this)),
    m_veinStorage(veinStorage),
    m_databaseFactory(factoryFunction),
    m_storageMode(storageMode)
{
    switch(storageMode) {
    case AbstractLoggerDB::STORAGE_MODE::TEXT:
        m_entityId = 2;
        m_entityName = QLatin1String("_LoggingSystem");
        qInfo() << "Created plaintext logger:" << m_entityName << "with id:" << m_entityId;
        break;
    case AbstractLoggerDB::STORAGE_MODE::BINARY:
        //use different id and entity name
        m_entityId = 200000;
        m_entityName = QLatin1String("_BinaryLoggingSystem");
        qInfo() <<  "Created binary logger:" << m_entityName << "with id:" << m_entityId;
        break;
    }

    connect(this, &DatabaseLogger::sigAttached, m_dPtr, &DataLoggerPrivate::initOnce);
    initModmanSessionComponent();

    m_asyncDatabaseThread.setObjectName("VFLoggerDBThread");

    m_schedulingTimer.setSingleShot(true);
    m_countdownUpdateTimer.setInterval(100);
    connect(&m_countdownUpdateTimer, &QTimer::timeout, this, &DatabaseLogger::onSchedulerCountdownToVein);

    m_batchedExecutionTimer.setSingleShot(false);
    m_batchedExecutionTimer.setInterval(5000);
    connect(&m_schedulingTimer, &QTimer::timeout, this, [this]() {
        setLoggingEnabled(false);
    });

}

DatabaseLogger::~DatabaseLogger()
{
    terminateCurrentDb();
    delete m_dPtr;
}

void DatabaseLogger::processEvent(QEvent *event)
{
    using namespace VeinEvent;
    using namespace VeinComponent;
    if(event->type()==CommandEvent::getQEventType()) {
        CommandEvent *cEvent = static_cast<CommandEvent *>(event);
        EventData *evData = cEvent->eventData();

        const bool isLogRunning = m_dbReady && m_loggingActive;
        if(evData->type() == ComponentData::dataType()) {
            ComponentData *cData = static_cast<ComponentData *>(evData);

            if(isLogRunning && cEvent->eventSubtype() == CommandEvent::EventSubtype::NOTIFICATION)
                addValueToDb(cData->newValue(), evData->entityId(), cData->componentName());

            else if(cEvent->eventSubtype() == CommandEvent::EventSubtype::TRANSACTION &&
                     evData->entityId() == m_entityId) {
                const QString componentName = cData->componentName();
                const QVariant newValue = cData->newValue();

                if(cData->eventCommand() == VeinComponent::ComponentData::Command::CCMD_SET) {
                    if(componentName == DataLoggerPrivate::loggedComponentsComponentName)
                        handleLoggedComponentsSetNotification(cData);
                    else if(componentName == DataLoggerPrivate::s_databaseFileComponentName) {
                        if(m_database == nullptr || newValue != m_databaseFilePath) {
                            if(newValue.toString().isEmpty()) //unsetting the file component = closing the database
                                closeDatabase();
                            else
                                onOpenDatabase(newValue.toString());
                        }

                        // a good place to reset selected sessionName - however db-open ends up with
                        QEvent* event = VfServerComponentSetter::generateEvent(m_entityId, DataLoggerPrivate::s_sessionNameComponentName,
                                                                               cData->oldValue(), QString());
                        emit sigSendEvent(event);
                        m_dbSessionName = "";
                    }
                    else if(componentName == DataLoggerPrivate::s_loggingEnabledComponentName) {
                        bool loggingEnabled = newValue.toBool();
                        if(loggingEnabled) {
                            if(checkConditionsForStartLog()) {
                                prepareLogging();
                                setLoggingEnabled(loggingEnabled);
                            }
                        }
                        else
                            setLoggingEnabled(loggingEnabled);
                    }
                    else if(componentName == DataLoggerPrivate::s_scheduledLoggingEnabledComponentName) {
                        bool scheduledLogging = newValue.toBool();
                        if(scheduledLogging != m_scheduledLogging) {
                            m_scheduledLogging = scheduledLogging;
                            setLoggingEnabled(false);
                            QEvent *event = VfServerComponentSetter::generateEvent(m_entityId, DataLoggerPrivate::s_scheduledLoggingEnabledComponentName,
                                                                                   cData->oldValue(), scheduledLogging);
                            emit sigSendEvent(event);
                        }
                    }
                    else if(componentName == DataLoggerPrivate::s_scheduledLoggingDurationComponentName) {
                        bool conversionOk = false;
                        const int logDurationMsecs = newValue.toInt(&conversionOk);
                        bool invalidTime = !conversionOk;
                        if(conversionOk) {
                            if(logDurationMsecs > 0) {
                                m_schedulingTimer.setInterval(logDurationMsecs);
                                if(isLogRunning)
                                    m_schedulingTimer.start();
                                QEvent *event = VfServerComponentSetter::generateEvent(m_entityId, DataLoggerPrivate::s_scheduledLoggingDurationComponentName,
                                                                                       cData->oldValue(), newValue);
                                emit sigSendEvent(event);
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
                        QEvent *event;
                        event = VfServerComponentSetter::generateEvent(m_entityId, DataLoggerPrivate::s_sessionNameComponentName,
                                                                       cData->oldValue(), newValue);
                        emit sigSendEvent(event);

                        QVariant sessionCustomerDataName = "";
                        if(!m_dbSessionName.isEmpty()) {
                            if(m_dbReady)
                                sessionCustomerDataName = handleVeinDbSessionNameSet(m_dbSessionName);
                            else
                                qWarning("Cannot set session '%s' - database is not ready yet!", qPrintable(m_dbSessionName));
                        }

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

                    event->accept();
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

void DatabaseLogger::writeCurrentStorageToDb()
{
    for(const int tmpEntityId : m_loggedValues.uniqueKeys()) {
        const QList<QString> tmpComponents = m_loggedValues.values(tmpEntityId);
        for(const QString &tmpComponentName : tmpComponents) {
            if(m_veinStorage->hasEntity(tmpEntityId)) { // is entity in storage?
                QStringList componentNamesToAdd;
                if(tmpComponentName == VLGlobalLabels::allComponentsName())
                    componentNamesToAdd = getComponentsFilteredForDb(tmpEntityId);
                else
                    componentNamesToAdd.append(tmpComponentName);

                for(const auto &componentToAdd : qAsConst(componentNamesToAdd)) {
                    const QVariant storedValue = m_veinStorage->getStoredValue(tmpEntityId, componentToAdd);
                    addValueToDb(storedValue, tmpEntityId, componentToAdd);
                }
            }
        }
    }
}

QStringList DatabaseLogger::getComponentsFilteredForDb(int entityId)
{
    QStringList retList = m_veinStorage->getEntityComponents(entityId);
    const QStringList componentsNoStore = VLGlobalLabels::noStoreComponents();
    for(const auto &noStoreLabel : componentsNoStore)
        retList.removeAll(noStoreLabel);
    return retList;
}

void DatabaseLogger::onSchedulerCountdownToVein()
{
    if(m_schedulingTimer.isActive()) {
        QEvent *event = VfServerComponentSetter::generateEvent(m_entityId, DataLoggerPrivate::s_scheduledLoggingCountdownComponentName,
                                                               QVariant(), m_schedulingTimer.remainingTime());
        emit sigSendEvent(event);
    }
}

void DatabaseLogger::statusTextToVein(const QString &status)
{
    if(m_loggerStatusText != status) {
        m_loggerStatusText = status;
        QEvent *event = VfServerComponentSetter::generateEvent(m_entityId, DataLoggerPrivate::s_loggingStatusTextComponentName,
                                                               QVariant(), status);
        emit sigSendEvent(event);
    }

}

void DatabaseLogger::prepareLogging()
{
    m_transactionId = m_database->addTransaction(m_transactionName, m_dbSessionName, m_contentSets, m_guiContext);
    m_database->addStartTime(m_transactionId, QDateTime::currentDateTime());
    writeCurrentStorageToDb();
}

int DatabaseLogger::entityId() const
{
    return m_entityId;
}

QString DatabaseLogger::entityName() const
{
    return m_entityName;
}

void DatabaseLogger::setLoggingEnabled(bool enabled)
{
    if(enabled != m_loggingActive) {
        if(enabled) {
            m_batchedExecutionTimer.start();
            if(m_scheduledLogging) {
                m_schedulingTimer.start();
                m_countdownUpdateTimer.start();
            }
            statusTextToVein("Logging data");
        }
        else {
            m_batchedExecutionTimer.stop();
            m_schedulingTimer.stop();
            m_countdownUpdateTimer.stop();
            emit m_dbCmdInterface.sigFlushToDb();
            statusTextToVein("Database loaded");
        }
        m_loggingActive = enabled;
        QEvent *event = VfServerComponentSetter::generateEvent(m_entityId, DataLoggerPrivate::s_loggingEnabledComponentName,
                                                               QVariant(), enabled);
        emit sigSendEvent(event);
    }
}

void DatabaseLogger::dbNameToVein(const QString &filePath)
{
    QEvent *event = VfServerComponentSetter::generateEvent(m_entityId, DataLoggerPrivate::s_databaseFileComponentName,
                                                           QVariant(), filePath);
    emit sigSendEvent(event);
}

bool DatabaseLogger::onOpenDatabase(const QString &filePath)
{
    m_databaseFilePath = filePath;
    qInfo("Open database %s", qPrintable(filePath));
    const bool validStorage = checkDBFilePath(filePath);
    if(validStorage) {
        terminateCurrentDb();
        m_database = m_databaseFactory();
        m_database->setStorageMode(m_storageMode);
        if(m_database->requiresOwnThread()) {
            m_database->moveToThread(&m_asyncDatabaseThread);
            m_asyncDatabaseThread.start();
        }

        m_dbCmdInterface.connectDb(m_database);
        connect(m_database, &AbstractLoggerDB::sigDatabaseReady, this, &DatabaseLogger::onDbReady);
        connect(m_database, &AbstractLoggerDB::sigDatabaseError, this, &DatabaseLogger::onDbError);
        connect(m_database, &AbstractLoggerDB::sigNewSessionList, this, &DatabaseLogger::updateSessionList);
        connect(&m_batchedExecutionTimer, &QTimer::timeout, m_database, &AbstractLoggerDB::runBatchedExecution);

        emit m_dbCmdInterface.sigOpenDatabase(filePath);
    }
    return validStorage;
}

void DatabaseLogger::terminateCurrentDb()
{
    m_asyncDatabaseThread.quit();
    m_asyncDatabaseThread.wait();
    delete m_database;
    m_database = nullptr;
}

void DatabaseLogger::closeDatabase()
{
    m_dbReady = false;
    setLoggingEnabled(false);

    terminateCurrentDb();
    if(m_deleteWatcher.directories().count()) {
        const QStringList watchedDirs = m_deleteWatcher.directories();
        for(const QString &watchDir : watchedDirs)
            m_deleteWatcher.removePath(watchDir);
        QObject::disconnect(&m_deleteWatcher, &QFileSystemWatcher::directoryChanged, this, &DatabaseLogger::checkDatabaseStillValid);
    }
    QEvent* event = VfServerComponentSetter::generateEvent(m_entityId, DataLoggerPrivate::s_databaseReadyComponentName, QVariant(), false);
    emit sigSendEvent(event);
    setLoggingEnabled(false);
    statusTextToVein("No database selected");

    QString closedDb = m_databaseFilePath;
    m_databaseFilePath.clear();
    dbNameToVein(m_databaseFilePath);

    // set CustomerData component empty
    event = VfServerComponentSetter::generateEvent(m_entityId, DataLoggerPrivate::s_customerDataComponentName,
                                                           QVariant(), QString());
    emit sigSendEvent(event);

    updateSessionList(QStringList());

    qInfo() << "Unloaded database:" << closedDb;
}

void DatabaseLogger::checkDatabaseStillValid()
{
    QFile dbFile(m_databaseFilePath);
    if(!dbFile.exists())
        onDbError(QString("Watcher detected database file %1 is gone!").arg(m_databaseFilePath));
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

void DatabaseLogger::onDbReady()
{
    QEvent* event = VfServerComponentSetter::generateEvent(m_entityId, DataLoggerPrivate::s_databaseReadyComponentName, QVariant(), true);
    emit sigSendEvent(event);

    setLoggingEnabled(false);

    // * To avoid fire storm on logging we watch file's dir
    // * For removable devices: mount-point's parent dir
    QFileInfo fileInfo(m_databaseFilePath);
    QStorageInfo storageInfo(fileInfo.absolutePath());
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
    if(m_deleteWatcher.directories().count())
        QObject::connect(&m_deleteWatcher, &QFileSystemWatcher::directoryChanged, this, &DatabaseLogger::checkDatabaseStillValid);
    if(unWatchedPaths.count())
        qWarning("Unwatched paths: %s", qPrintable(unWatchedPaths.join(QStringLiteral(" + "))));

    m_dbReady = true;
    statusTextToVein("Database loaded");
    dbNameToVein(m_databaseFilePath);
}

void DatabaseLogger::onDbError(QString errorMsg)
{
    qWarning() << errorMsg;
    closeDatabase();
    statusTextToVein("Database error");
    emit sigDatabaseError(errorMsg);
}

QString DatabaseLogger::getEntityName(int entityId) const
{
    return m_veinStorage->getStoredValue(entityId, "EntityName").toString();
}

QVariant DatabaseLogger::RPC_deleteSession(QVariantMap parameters)
{
    QString session = parameters["p_session"].toString();
    QVariant retVal = m_database->deleteSession(session);
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

void DatabaseLogger::handleLoggedComponentsSetNotification(VeinComponent::ComponentData *cData)
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

void DatabaseLogger::addLoggerEntry(int entityId, const QString &componentName)
{
    if(m_loggedValues.contains(entityId, componentName) == false)
        m_loggedValues.insert(entityId, componentName);
}

void DatabaseLogger::clearLoggerEntries()
{
    m_loggedValues.clear();
}

QString DatabaseLogger::handleVeinDbSessionNameSet(QString sessionName)
{
    QString sessionCustomerDataName;
    if(!m_database->hasSessionName(sessionName)) {
        QMultiHash<int, QString> tmpStaticComps;
        // Add customer data once per session
        if(m_veinStorage->hasEntity(200))
            for(const QString &comp : getComponentsFilteredForDb(200))
                tmpStaticComps.insert(200, comp);
        // Add status module once per session
        if(m_veinStorage->hasEntity(1150))
            for(const QString &comp : getComponentsFilteredForDb(1150))
                tmpStaticComps.insert(1150, comp);

        QList<DatabaseCommandInterface::ComponentInfo> componentsAddedOncePerSession;
        for(const int tmpEntityId : tmpStaticComps.uniqueKeys()) { //only process once for every entity
            const QList<QString> tmpComponents = tmpStaticComps.values(tmpEntityId);
            for(const QString &tmpComponentName : tmpComponents) {
                DatabaseCommandInterface::ComponentInfo component = {
                    tmpEntityId,
                    getEntityName(tmpEntityId),
                    tmpComponentName,
                    m_veinStorage->getStoredValue(tmpEntityId, tmpComponentName),
                    QDateTime::currentDateTime()
                };
                componentsAddedOncePerSession.append(component);
            }
        }

        sessionCustomerDataName = m_veinStorage->getStoredValue(200, "FileSelected").toString();
        emit m_dbCmdInterface.sigAddSession(sessionName, componentsAddedOncePerSession);
    }
    else
        sessionCustomerDataName = m_database->readSessionComponent(sessionName,"CustomerData", "FileSelected").toString();
    return sessionCustomerDataName;
}

bool DatabaseLogger::checkConditionsForStartLog()
{
    bool validConditions = true;
    if(!m_dbReady) {
        validConditions = false;
        qWarning("Logging requires a database!");
    }
    if(m_dbSessionName.isEmpty()) {
        validConditions = false;
        qWarning("Logging requires a valid sessionName!");
    }
    if(m_transactionName.isEmpty()) {
        validConditions = false;
        qWarning("Logging requires a valid transactionName!");
    }
    return validConditions;
}

void DatabaseLogger::initModmanSessionComponent()
{
    m_modmanSessionComponent = m_veinStorage->getFutureComponent(0, "Session");
    connect(m_modmanSessionComponent.get(), &VeinEvent::StorageComponentInterface::sigValueChange,
            this, &DatabaseLogger::onModmanSessionChange);

    if(m_modmanSessionComponent->getValue().isValid())
        onModmanSessionChange(m_modmanSessionComponent->getValue());
}

bool DatabaseLogger::checkDBFilePath(const QString &dbFilePath)
{
    bool retVal = false;
    QFileInfo fInfo(dbFilePath);
    if(!fInfo.isRelative()) {
        // try to create path
        if(!fInfo.absoluteDir().exists()) {
            QDir dir;
            dir.mkpath(fInfo.absoluteDir().path());
        }
        if(fInfo.absoluteDir().exists()) {
            if(fInfo.isFile() || fInfo.exists() == false)
                retVal = true;
            else
                onDbError(QString("Path is not a valid file location: %1").arg(dbFilePath));
        }
        else
            onDbError(QString("Parent directory for path does not exist: %1").arg(dbFilePath));
    }
    else
        onDbError(QString("Relative paths are not accepted: %1").arg(dbFilePath));
    return retVal;
}

void DatabaseLogger::addValueToDb(const QVariant newValue, const int entityId, const QString componentName)
{
    if(isLoggedComponent(entityId, componentName)) {
        QString entityName = getEntityName(entityId);
        DatabaseCommandInterface::ComponentInfo info = { entityId, entityName, componentName, newValue, QDateTime::currentDateTime() };
        emit m_dbCmdInterface.sigAddLoggedValue(m_dbSessionName, QVector<int>() << m_transactionId, info);
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
