#include "vl_databaselogger.h"
#include "loggerstatictexts.h"
#include "loggercontentsetconfig.h"
#include <vcmp_entitydata.h>
#include <vcmp_errordata.h>
#include <vf-cpp-rpc.h>
#include <vf_client_component_setter.h>
#include <vf_server_component_setter.h>
#include <QHash>
#include <QDir>
#include <QStorageInfo>
#include <QJsonDocument>

Q_DECLARE_METATYPE(VeinLogger::ComponentInfo)

namespace VeinLogger
{
DatabaseLogger::DatabaseLogger(VeinStorage::AbstractEventSystem *veinStorage,
                               DBFactory factoryFunction,
                               QObject *parent,
                               QList<int> entitiesWithAllComponentsStoredAlways,
                               AbstractLoggerDB::STORAGE_MODE storageMode) :
    VeinEvent::EventSystem(parent),
    m_veinStorage(veinStorage),
    m_databaseFactory(factoryFunction),
    m_storageMode(storageMode),
    m_loggedComponents(entitiesWithAllComponentsStoredAlways)
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

    connect(this, &DatabaseLogger::sigAttached, this, &DatabaseLogger::initOnce);
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
}

void VeinLogger::DatabaseLogger::handleContentSetsChange(const QVariant oldValue, const QVariant newValue)
{
    m_contentSets = newValue.toStringList();
    QVariantMap loggedComponents = LoggerContentSetConfig::componentFromContentSets(m_contentSets);
    m_loggedComponents.clear();

    // Client: we choose same as a client would - it can still access availableContentSets
    QEvent* event = VfClientComponentSetter::generateEvent(m_entityId, LoggerStaticTexts::loggedComponentsComponentName,
                                                           QVariant(), loggedComponents);
    emit sigSendEvent(event);

    event = VfServerComponentSetter::generateEvent(m_entityId, LoggerStaticTexts::s_currentContentSetsComponentName,
                                                   oldValue, newValue);
    emit sigSendEvent(event);
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
            const QString componentName = cData->componentName();

            if(isLogRunning && cEvent->eventSubtype() == CommandEvent::EventSubtype::NOTIFICATION)
                addValueToDb(cData->newValue(), evData->entityId(), componentName);

            else if(cEvent->eventSubtype() == CommandEvent::EventSubtype::TRANSACTION &&
                     evData->entityId() == m_entityId) {
                const QVariant newValue = cData->newValue();

                if(cData->eventCommand() == VeinComponent::ComponentData::Command::CCMD_SET) {
                    if(componentName == LoggerStaticTexts::loggedComponentsComponentName)
                        handleLoggedComponentsSetNotification(cData);
                    else if(componentName == LoggerStaticTexts::s_databaseFileComponentName) {
                        if(m_database == nullptr || newValue != m_databaseFilePath) {
                            if(newValue.toString().isEmpty()) //unsetting the file component = closing the database
                                closeDatabase();
                            else
                                openDatabase(newValue.toString());
                        }

                        // a good place to reset selected sessionName - however db-open ends up with
                        QEvent* event = VfServerComponentSetter::generateEvent(m_entityId, LoggerStaticTexts::s_sessionNameComponentName,
                                                                               cData->oldValue(), QString());
                        emit sigSendEvent(event);
                        m_dbSessionName = "";
                    }
                    else if(componentName == LoggerStaticTexts::s_loggingEnabledComponentName) {
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
                    else if(componentName == LoggerStaticTexts::s_scheduledLoggingEnabledComponentName) {
                        bool scheduledLogging = newValue.toBool();
                        if(scheduledLogging != m_scheduledLogging) {
                            m_scheduledLogging = scheduledLogging;
                            setLoggingEnabled(false);
                            QEvent *event = VfServerComponentSetter::generateEvent(m_entityId, LoggerStaticTexts::s_scheduledLoggingEnabledComponentName,
                                                                                   cData->oldValue(), scheduledLogging);
                            emit sigSendEvent(event);
                        }
                    }
                    else if(componentName == LoggerStaticTexts::s_scheduledLoggingDurationComponentName) {
                        bool conversionOk = false;
                        const int logDurationMsecs = newValue.toInt(&conversionOk);
                        bool invalidTime = !conversionOk;
                        if(conversionOk) {
                            if(logDurationMsecs > 0) {
                                m_schedulingTimer.setInterval(logDurationMsecs);
                                if(isLogRunning)
                                    m_schedulingTimer.start();
                                QEvent *event = VfServerComponentSetter::generateEvent(m_entityId, LoggerStaticTexts::s_scheduledLoggingDurationComponentName,
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
                    else if(componentName == LoggerStaticTexts::s_sessionNameComponentName) {
                        m_dbSessionName = newValue.toString();
                        QEvent *event = VfServerComponentSetter::generateEvent(m_entityId, LoggerStaticTexts::s_sessionNameComponentName,
                                                                               cData->oldValue(), newValue);
                        emit sigSendEvent(event);

                        if (!m_dbSessionName.isEmpty()) {
                            if(m_dbReady)
                                handleVeinDbSessionNameSet(m_dbSessionName);
                            else
                                qWarning("Cannot set session '%s' - database is not ready yet!", qPrintable(m_dbSessionName));
                        }
                    }
                    else if(componentName == LoggerStaticTexts::s_guiContextComponentName) {
                        m_guiContext = newValue.toString();
                        QEvent *event = VfServerComponentSetter::generateEvent(m_entityId, LoggerStaticTexts::s_guiContextComponentName,
                                                                               cData->oldValue(), newValue);
                        emit sigSendEvent(event);
                    }
                    else if(componentName == LoggerStaticTexts::s_transactionNameComponentName) {
                        m_transactionName = newValue.toString();
                        QEvent* event = VfServerComponentSetter::generateEvent(m_entityId, LoggerStaticTexts::s_transactionNameComponentName,
                                                                               cData->oldValue(), newValue);
                        emit sigSendEvent(event);
                    }
                    else if(componentName == LoggerStaticTexts::s_currentContentSetsComponentName) {
                        handleContentSetsChange(cData->oldValue(), newValue);
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
                if(m_rpcSimplifiedList.contains(rpcData->procedureName())){
                    const QUuid callId = rpcData->invokationData().value(VeinComponent::RemoteProcedureData::s_callIdString).toUuid();
                    m_rpcSimplifiedList[rpcData->procedureName()]->callFunction(callId,cEvent->peerId(),rpcData->invokationData());
                    cEvent->accept();
                }
                else if(!cEvent->isAccepted()) {
                    qWarning() << "No remote procedure with entityId:" << m_entityId << "name:" << rpcData->procedureName();
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
    const VeinStorage::AbstractDatabase* storageDb = m_veinStorage->getDb();
    const QList<int> entities = m_loggedComponents.getEntities();
    for(const int entityId : entities) {
        QStringList components;
        if(m_loggedComponents.areAllComponentsStored(entityId))
            components = getComponentsFilteredForDb(entityId);
        else
            components = m_loggedComponents.getComponents(entityId);
        for(const QString &component : qAsConst(components)) {
            if(storageDb->hasStoredValue(entityId, component)) {
                const QVariant storedValue = storageDb->getStoredValue(entityId, component);
                addValueToDb(storedValue, entityId, component);
            }
        }
    }
}

QStringList DatabaseLogger::getComponentsFilteredForDb(int entityId)
{
    QStringList fullList = m_veinStorage->getDb()->getComponentList(entityId);
    return LoggedComponents::removeNotStoredOnEntitiesStoringAllComponents(fullList);
}

void DatabaseLogger::onSchedulerCountdownToVein()
{
    if(m_schedulingTimer.isActive()) {
        QEvent *event = VfServerComponentSetter::generateEvent(m_entityId, LoggerStaticTexts::s_scheduledLoggingCountdownComponentName,
                                                               QVariant(), m_schedulingTimer.remainingTime());
        emit sigSendEvent(event);
    }
}

void DatabaseLogger::statusTextToVein(const QString &status)
{
    if(m_loggerStatusText != status) {
        m_loggerStatusText = status;
        QEvent *event = VfServerComponentSetter::generateEvent(m_entityId, LoggerStaticTexts::s_loggingStatusTextComponentName,
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

AbstractLoggerDB *DatabaseLogger::getDb() const
{
    return m_database;
}

bool DatabaseLogger::isDatabaseReady() const
{
    return m_dbReady;
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
            getDb()->startFlushToDb();
            statusTextToVein("Database loaded");
        }
        m_loggingActive = enabled;
        QEvent *event = VfServerComponentSetter::generateEvent(m_entityId, LoggerStaticTexts::s_loggingEnabledComponentName,
                                                               QVariant(), enabled);
        emit sigSendEvent(event);
    }
}

void DatabaseLogger::dbNameToVein(const QString &filePath)
{
    QEvent *event = VfServerComponentSetter::generateEvent(m_entityId, LoggerStaticTexts::s_databaseFileComponentName,
                                                           QVariant(), filePath);
    emit sigSendEvent(event);
}

void DatabaseLogger::openDatabase(const QString &filePath)
{
    m_databaseFilePath = filePath;
    qInfo("Open database %s", qPrintable(filePath));
    if(checkDBFilePath(filePath)) {
        terminateCurrentDb();
        m_database = m_databaseFactory();
        m_database->setStorageMode(m_storageMode);
        if(m_database->requiresOwnThread()) {
            m_database->moveToThread(&m_asyncDatabaseThread);
            m_asyncDatabaseThread.start();
        }

        connect(m_database, &AbstractLoggerDB::sigDatabaseReady, this, &DatabaseLogger::onDbReady, Qt::QueuedConnection);
        connect(m_database, &AbstractLoggerDB::sigDatabaseError, this, &DatabaseLogger::onDbError, Qt::QueuedConnection);
        connect(m_database, &AbstractLoggerDB::sigNewSessionList, this, &DatabaseLogger::updateSessionList, Qt::QueuedConnection);
        connect(m_database, &AbstractLoggerDB::sigDeleteSessionCompleted, this, &DatabaseLogger::onDeleteSessionCompleted, Qt::QueuedConnection);
        connect(this, &DatabaseLogger::sigAddLoggedValue, m_database, &AbstractLoggerDB::addLoggedValue, Qt::QueuedConnection);
        connect(this, &DatabaseLogger::sigAddSession, m_database, &AbstractLoggerDB::addSession, Qt::QueuedConnection);
        connect(this, &DatabaseLogger::sigOpenDatabase, m_database, &AbstractLoggerDB::onOpen, Qt::QueuedConnection);

        connect(&m_batchedExecutionTimer, &QTimer::timeout, m_database, &AbstractLoggerDB::onFlushToDb, Qt::QueuedConnection);

        emit sigOpenDatabase(filePath);
    }
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
    if(m_databaseFilePath.isEmpty())
        return;
    m_dbReady = false;
    setLoggingEnabled(false);

    terminateCurrentDb();
    if(m_deleteWatcher.directories().count()) {
        const QStringList watchedDirs = m_deleteWatcher.directories();
        for(const QString &watchDir : watchedDirs)
            m_deleteWatcher.removePath(watchDir);
        QObject::disconnect(&m_deleteWatcher, &QFileSystemWatcher::directoryChanged, this, &DatabaseLogger::checkDatabaseStillValid);
    }
    QEvent* event = VfServerComponentSetter::generateEvent(m_entityId, LoggerStaticTexts::s_databaseReadyComponentName, QVariant(), false);
    emit sigSendEvent(event);
    setLoggingEnabled(false);
    statusTextToVein("No database selected");

    QString closedDb = m_databaseFilePath;
    m_databaseFilePath.clear();
    dbNameToVein(m_databaseFilePath);

    // set CustomerData component empty
    event = VfServerComponentSetter::generateEvent(m_entityId, LoggerStaticTexts::s_customerDataComponentName,
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
    m_existingSessions = sessionNames;
    QEvent* event = VfServerComponentSetter::generateEvent(m_entityId, LoggerStaticTexts::s_existingSessionsComponentName,
                                                           QVariant(), sessionNames);
    emit sigSendEvent(event);
}

void DatabaseLogger::onDeleteSessionCompleted(QUuid callId, bool success, QString errorMsg, QStringList newSessionsList)
{
    Q_UNUSED(callId)
    Q_UNUSED(errorMsg)
    // check if deleted session is current Session and if it is set sessionName empty
    // We will not check retVal here. If something goes wrong and the session is still available the
    // user can choose it again without risking undefined behavior.
    if(!newSessionsList.contains(m_dbSessionName)) {
        VeinComponent::ComponentData *sessionNameCData = new VeinComponent::ComponentData();
        sessionNameCData ->setEntityId(m_entityId);
        sessionNameCData ->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
        sessionNameCData ->setComponentName(LoggerStaticTexts::s_sessionNameComponentName);
        sessionNameCData ->setNewValue(QString());
        sessionNameCData ->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
        sessionNameCData ->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);
        m_dbSessionName = "";
        emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, sessionNameCData));
    }
    if(success)
        updateSessionList(newSessionsList);
    emit sigDeleteSessionCompleted(callId, success, errorMsg);
}

void VeinLogger::DatabaseLogger::emptyCurrentContentSets()
{
    if(m_veinStorage->getDb()->hasStoredValue(m_entityId, LoggerStaticTexts::s_currentContentSetsComponentName)) {
        QVariant oldValue = m_veinStorage->getDb()->getStoredValue(m_entityId, LoggerStaticTexts::s_currentContentSetsComponentName);
        QEvent* event = VfServerComponentSetter::generateEvent(m_entityId, LoggerStaticTexts::s_currentContentSetsComponentName,
                                                               oldValue, QVariantList());
        emit sigSendEvent(event);
    }
}

void DatabaseLogger::emptyLoggedComponents()
{
    if(m_veinStorage->getDb()->hasStoredValue(m_entityId, LoggerStaticTexts::loggedComponentsComponentName)) {
        QVariant oldValue = m_veinStorage->getDb()->getStoredValue(m_entityId, LoggerStaticTexts::loggedComponentsComponentName);
        QEvent* event = VfServerComponentSetter::generateEvent(m_entityId, LoggerStaticTexts::loggedComponentsComponentName,
                                                               oldValue, QVariantMap());
        emit sigSendEvent(event);
    }
}

void DatabaseLogger::onModmanSessionChange(QVariant newSession)
{
    setLoggingEnabled(false);
    for (auto &env : LoggerContentSetConfig::getConfigEnvironment())
        env.m_loggerContentHandler->setSession(newSession.toString());
    const QStringList availContentSetStrings = LoggerContentSetConfig::getAvailableContentSets();

    // Avoid QVariant type cast - is there a more simple way?
    QVariantList availContentSets;
    for(const QString &availContentSetString : availContentSetStrings)
        availContentSets.append(availContentSetString);

    QEvent* event = VfServerComponentSetter::generateEvent(m_entityId, LoggerStaticTexts::s_availableContentSetsComponentName,
                                                           QVariant(), availContentSets);
    emit sigSendEvent(event);

    if(!newSession.toString().isEmpty()) {
        emptyCurrentContentSets();
        emptyLoggedComponents();
    }
}

void DatabaseLogger::onDbReady()
{
    QEvent* event = VfServerComponentSetter::generateEvent(m_entityId, LoggerStaticTexts::s_databaseReadyComponentName, QVariant(), true);
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
    return m_veinStorage->getDb()->getStoredValue(entityId, "EntityName").toString();
}

void DatabaseLogger::initOnce()
{
    Q_ASSERT(m_initDone == false);
    if(m_initDone == false) {
        VeinComponent::EntityData *systemData = new VeinComponent::EntityData();
        systemData->setCommand(VeinComponent::EntityData::Command::ECMD_ADD);
        systemData->setEntityId(m_entityId);
        VeinEvent::CommandEvent *systemEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, systemData);
        emit sigSendEvent(systemEvent);

        VeinComponent::ComponentData *initialData = nullptr;
        QHash<QString, QVariant> componentData;
        componentData.insert(LoggerStaticTexts::s_entityNameComponentName, entityName());
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
            initialData->setEntityId(m_entityId);
            initialData->setCommand(VeinComponent::ComponentData::Command::CCMD_ADD);
            initialData->setComponentName(componentName);
            initialData->setNewValue(componentData.value(componentName));
            initialData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
            initialData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

            systemEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, initialData);
            emit sigSendEvent(systemEvent);
        }

        m_rpcDeleteSession = std::make_shared<RpcDeleteSession>(this, m_entityId);
        m_rpcSimplifiedList[m_rpcDeleteSession->getSignature()] = m_rpcDeleteSession;
        m_rpcDisplaySessionsInfos = std::make_shared<RpcDisplaySessionsInfos>(this, m_entityId);
        m_rpcSimplifiedList[m_rpcDisplaySessionsInfos->getSignature()] = m_rpcDisplaySessionsInfos;
        m_rpcDeleteTransaction = std::make_shared<RpcDeleteTransaction>(this, m_entityId);
        m_rpcSimplifiedList[m_rpcDeleteTransaction->getSignature()] = m_rpcDeleteTransaction;
        m_rpcListAllSessions = std::make_shared<RpcListAllSessions>(this, m_entityId);
        m_rpcSimplifiedList[m_rpcListAllSessions->getSignature()] = m_rpcListAllSessions;
        m_rpcDisplayActualValues = std::make_shared<RpcDisplayActualValues>(this, m_entityId);
        m_rpcSimplifiedList[m_rpcDisplayActualValues->getSignature()] = m_rpcDisplayActualValues;

        m_initDone = true;
    }
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
    m_loggedComponents.clear();

    QVariantMap entityComponentMap = newValue.toMap();
    QStringList entityIdsStr = entityComponentMap.keys();
    for(const auto &entityIdStr : entityIdsStr) {
        QVariantList componentList = entityComponentMap[entityIdStr].toList();
        if(componentList.size()) {
            for(auto &component : qAsConst(componentList))
                m_loggedComponents.addComponent(entityIdStr.toInt(), component.toString());
        }
        else
            m_loggedComponents.addAllComponents(entityIdStr.toInt());
    }
}

void DatabaseLogger::handleVeinDbSessionNameSet(QString sessionName)
{
    if(!m_database->hasSessionName(sessionName)) {
        QMultiHash<int, QString> tmpStaticComps;
        const VeinStorage::AbstractDatabase* storageDb = m_veinStorage->getDb();
        // Add customer data once per session
        if(storageDb->hasEntity(200))
            for(const QString &comp : getComponentsFilteredForDb(200))
                tmpStaticComps.insert(200, comp);
        // Add status module once per session
        if(storageDb->hasEntity(1150))
            for(const QString &comp : getComponentsFilteredForDb(1150))
                tmpStaticComps.insert(1150, comp);

        QList<ComponentInfo> componentsAddedOncePerSession;
        for(const int tmpEntityId : tmpStaticComps.uniqueKeys()) { //only process once for every entity
            const QList<QString> tmpComponents = tmpStaticComps.values(tmpEntityId);
            for(const QString &tmpComponentName : tmpComponents) {
                ComponentInfo component = {
                    tmpEntityId,
                    getEntityName(tmpEntityId),
                    tmpComponentName,
                    storageDb->getStoredValue(tmpEntityId, tmpComponentName),
                    QDateTime::currentDateTime()
                };
                componentsAddedOncePerSession.append(component);
            }
        }
        emit sigAddSession(sessionName, componentsAddedOncePerSession);
    }
}

bool DatabaseLogger::checkConditionsForStartLog() const
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
    m_modmanSessionComponent = m_veinStorage->getDb()->getFutureComponent(0, "Session");
    connect(m_modmanSessionComponent.get(), &VeinStorage::AbstractComponent::sigValueChange,
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
    if(m_loggedComponents.isLoggedComponent(entityId, componentName)) {
        QString entityName = getEntityName(entityId);
        ComponentInfo info = { entityId, entityName, componentName, newValue, QDateTime::currentDateTime() };
        emit sigAddLoggedValue(m_dbSessionName, QVector<int>() << m_transactionId, info);
    }
}

}
