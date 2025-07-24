#include "vl_databaselogger.h"
#include "loggerstatictexts.h"
#include "loggercontentsetconfig.h"
#include "databasefilepathchecker.h"
#include "taskdbaddsession.h"
#include <vcmp_entitydata.h>
#include <vcmp_errordata.h>
#include <vf-cpp-rpc.h>
#include <vf_client_component_setter.h>
#include <vf_server_component_setter.h>
#include <vf_server_component_add.h>
#include <QHash>
#include <QJsonDocument>

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
    m_entitiesWithAllComponentsStoredAlways(entitiesWithAllComponentsStoredAlways),
    m_loggedComponentsHelper(
          std::make_unique<LoggerContentsetsHelper>(
                m_entitiesWithAllComponentsStoredAlways,
                m_contentSets))

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

void DatabaseLogger::processEvent(QEvent *eventToProcess)
{
    using namespace VeinEvent;
    using namespace VeinComponent;
    if(eventToProcess->type()==CommandEvent::getQEventType()) {
        CommandEvent *cEvent = static_cast<CommandEvent *>(eventToProcess);
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
                        handleVeinLoggedComponentsChange(cData);
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
                            const QStringList errs = checkConditionsForStartLog();
                            if(errs.isEmpty()) {
                                prepareLogging();
                                setLoggingEnabled(loggingEnabled);
                            }
                            else
                                qWarning() << errs;
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

                    eventToProcess->accept();
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
    EntityComponentData entityCompData = m_loggedComponentsHelper->getCurrentData(m_veinStorage);
    for (auto iterEntity = entityCompData.constBegin(); iterEntity != entityCompData.constEnd(); ++iterEntity) {
        int entId = iterEntity.key();
        const QList<Component> EntityComponentData = iterEntity.value();
        for (const Component &componentData : EntityComponentData) {
            QString entName = getEntityName(entId);
            ComponentInfo info = { entId, entName, componentData.name, componentData.data, QDateTime::currentDateTime() };
            emit sigAddLoggedValue(m_dbSessionName, QVector<int>() << m_transactionId, info);
        }
    }
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
    m_database->updateTransactionStartTime(m_transactionId, QDateTime::currentDateTime());
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

AbstractLoggerDBPtr DatabaseLogger::getDb() const
{
    return m_database;
}

bool DatabaseLogger::isDatabaseReady() const
{
    return m_dbReady;
}

void DatabaseLogger::startTakeSnapshot(const QUuid &callId,
                                       StartTransactionParam param)
{

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
    const QString errMsg = DatabaseFilePathChecker::createAndCheckDir(filePath);
    if(!errMsg.isEmpty()) {
        onDbError(errMsg);
        return;
    }

    terminateCurrentDb();
    m_database = m_databaseFactory();
    m_database->setStorageMode(m_storageMode);
    if(m_database->requiresOwnThread()) {
        m_database->moveToThread(&m_asyncDatabaseThread);
        m_asyncDatabaseThread.start();
    }

    connect(m_database.get(), &AbstractLoggerDB::sigDatabaseReady,
            this, &DatabaseLogger::onDbReady, Qt::QueuedConnection);
    connect(m_database.get(), &AbstractLoggerDB::sigDatabaseError,
            this, &DatabaseLogger::onDbError, Qt::QueuedConnection);
    connect(m_database.get(), &AbstractLoggerDB::sigNewSessionList,
            this, &DatabaseLogger::updateSessionList, Qt::QueuedConnection);
    connect(m_database.get(), &AbstractLoggerDB::sigDeleteSessionCompleted,
            this, &DatabaseLogger::onDeleteSessionCompleted, Qt::QueuedConnection);
    connect(this, &DatabaseLogger::sigAddLoggedValue,
            m_database.get(), &AbstractLoggerDB::addLoggedValue, Qt::QueuedConnection);
    connect(this, &DatabaseLogger::sigOpenDatabase,
            m_database.get(), &AbstractLoggerDB::onOpen, Qt::QueuedConnection);

    connect(&m_batchedExecutionTimer, &QTimer::timeout,
            m_database.get(), &AbstractLoggerDB::onFlushToDb, Qt::QueuedConnection);

    emit sigOpenDatabase(filePath);
}

void DatabaseLogger::terminateCurrentDb()
{
    m_asyncDatabaseThread.quit();
    m_asyncDatabaseThread.wait();
    m_database.reset();
}

void DatabaseLogger::closeDatabase()
{
    if(m_databaseFilePath.isEmpty())
        return;
    m_dbReady = false;
    setLoggingEnabled(false);

    terminateCurrentDb();
    m_dbFileWatcher.reset();
    QEvent* event = VfServerComponentSetter::generateEvent(m_entityId, LoggerStaticTexts::s_databaseReadyComponentName, QVariant(), false);
    emit sigSendEvent(event);
    setLoggingEnabled(false);
    statusTextToVein("No database selected");

    QString closedDb = m_databaseFilePath;
    m_databaseFilePath.clear();
    dbNameToVein(m_databaseFilePath);

    updateSessionList(QStringList());

    qInfo() << "Unloaded database:" << closedDb;
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
    // check if deleted session is current session (or got lost..)
    // If so set sessionName empty
    if(!newSessionsList.contains(m_dbSessionName)) {
        QEvent *event = VfServerComponentSetter::generateEvent(m_entityId, LoggerStaticTexts::s_sessionNameComponentName,
                                                               m_dbSessionName, QString());
        m_dbSessionName = "";
        emit sigSendEvent(event);
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
        env.m_loggerContentHandler->setModmanSession(newSession.toString());
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

    m_dbFileWatcher = std::make_unique<DatabaseFileWatcher>();
    connect(m_dbFileWatcher.get(), &DatabaseFileWatcher::sigFileError, this, [&]() {
        onDbError(QString("Watcher detected database file %1 is gone!").arg(m_databaseFilePath));
    });
    m_dbFileWatcher->startWatch(m_databaseFilePath);

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
        componentData.insert(LoggerStaticTexts::loggedComponentsComponentName, QVariantMap());
        componentData.insert(LoggerStaticTexts::s_sessionNameComponentName, QString());
        componentData.insert(LoggerStaticTexts::s_guiContextComponentName, QString());
        componentData.insert(LoggerStaticTexts::s_transactionNameComponentName, QString());
        componentData.insert(LoggerStaticTexts::s_currentContentSetsComponentName, QVariantList());
        componentData.insert(LoggerStaticTexts::s_availableContentSetsComponentName, QVariantList());

        for (auto iter = componentData.keyBegin(); iter != componentData.keyEnd(); iter++) {
            const QString &componentName = *iter;
            QEvent *addEvent = VfServerComponentAdd::generateEvent(m_entityId, componentName, componentData.value(componentName));
            emit sigSendEvent(addEvent);
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
        m_rpcCreateSnapshot = std::make_shared<RpcCreateSnapshot>(this, m_entityId);
        m_rpcSimplifiedList[m_rpcCreateSnapshot->getSignature()] = m_rpcCreateSnapshot;

        m_initDone = true;
    }
}

void VeinLogger::DatabaseLogger::handleContentSetsChange(const QVariant oldValue, const QVariant newValue)
{
    m_contentSets = newValue.toStringList();
    m_loggedComponentsHelper = std::make_unique<LoggerContentsetsHelper>(
        m_entitiesWithAllComponentsStoredAlways,
        m_contentSets);

    QEvent* event = VfServerComponentSetter::generateEvent(m_entityId, LoggerStaticTexts::s_currentContentSetsComponentName,
                                                           oldValue, newValue);
    emit sigSendEvent(event);

    // Update LoggedComponents (TODO remove)
    QVariantMap loggedComponents = LoggerContentSetConfig::componentFromContentSets(m_contentSets);
    // Client: we choose same as a client would - it can still access availableContentSets
    event = VfClientComponentSetter::generateEvent(m_entityId, LoggerStaticTexts::loggedComponentsComponentName,
                                                           QVariant(), loggedComponents);
    emit sigSendEvent(event);

}

// for (test?) compatibility -> remove LoggedComponents later
void DatabaseLogger::handleVeinLoggedComponentsChange(VeinComponent::ComponentData *cData)
{
    QVariant oldValue = cData->oldValue();
    QVariant newValue = cData->newValue();
    if(oldValue != newValue) {
        if(static_cast<QMetaType::Type>(newValue.type()) != QMetaType::QVariantMap) {
            qWarning("DatabaseLogger::handleLoggedComponentsChange: wrong type");
            return;
        }
        m_loggedComponentsHelper = std::make_unique<LoggerContentsetsHelper>(
            m_entitiesWithAllComponentsStoredAlways,
            newValue.toMap());
    }

    QEvent* event = VfServerComponentSetter::generateEvent(m_entityId, cData->componentName(), oldValue, newValue);
    emit sigSendEvent(event);
}

void DatabaseLogger::handleVeinDbSessionNameSet(QString sessionName)
{
    m_taskDbAddSession = std::make_unique<TaskDbAddSession>(m_database,
                                                            m_veinStorage,
                                                            sessionName,
                                                            std::make_shared<int>(-1));
    // Fire & forget for now
    m_taskDbAddSession->start();
}

QStringList DatabaseLogger::checkConditionsForStartLog() const
{
    QStringList errMsgs;
    if(!m_dbReady)
        errMsgs.append("Logging requires a database!");
    if(m_dbSessionName.isEmpty())
        errMsgs.append("Logging requires a valid sessionName!");
    if(m_transactionName.isEmpty())
        errMsgs.append("Logging requires a valid transactionName!");
    return errMsgs;
}

void DatabaseLogger::initModmanSessionComponent()
{
    m_modmanSessionComponent = m_veinStorage->getDb()->getFutureComponent(0, "Session");
    connect(m_modmanSessionComponent.get(), &VeinStorage::AbstractComponent::sigValueChange,
            this, &DatabaseLogger::onModmanSessionChange);

    if(m_modmanSessionComponent->getValue().isValid())
        onModmanSessionChange(m_modmanSessionComponent->getValue());
}

void DatabaseLogger::addValueToDb(const QVariant &newValue, const int entityId, const QString &componentName)
{
    if (m_loggedComponentsHelper->isLoggedComponent(entityId, componentName)) {
        QString entName = getEntityName(entityId);
        ComponentInfo info = { entityId, entName, componentName, newValue, QDateTime::currentDateTime() };
        emit sigAddLoggedValue(m_dbSessionName, QVector<int>() << m_transactionId, info);
    }
}

}
