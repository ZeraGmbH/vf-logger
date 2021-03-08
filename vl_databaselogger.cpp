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
#include <veinmodulerpc.h>
#include <QJsonDocument>
#include <vl_databaseloggerprivate.h>

Q_LOGGING_CATEGORY(VEIN_LOGGER, VEIN_DEBUGNAME_LOGGER)

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
    QString mimeInfo;
    qint64 fileSize = 0;
    // Mime & size are set (again) in database-ready - there we have a file definitely
    QFileInfo fileInfo(t_filePath);
    if(fileInfo.exists()) {
        fileSize = fileInfo.size();
        QMimeDatabase mimeDB;
        mimeInfo = mimeDB.mimeTypeForFile(fileInfo, QMimeDatabase::MatchContent).name();
    }
    fileInfoData.insert(DataLoggerPrivate::s_databaseFileMimeTypeComponentName, mimeInfo);
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
        m_dPtr->updateDBStorageInfo();
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
        connect(this, SIGNAL(sigAddLoggedValue(QString,QVector<int>,int,QString,QVariant,QDateTime)), m_dPtr->m_database, SLOT(addLoggedValue(QString,QVector<int>,int,QString,QVariant,QDateTime)));
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

    m_dPtr->updateDBStorageInfo();

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

bool DatabaseLogger::processEvent(QEvent *t_event)
{
    using namespace VeinEvent;
    using namespace VeinComponent;

    bool retVal = false;

    if(t_event->type()==CommandEvent::eventType()) {
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
                    retVal = true;
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
                        retVal = true;
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
                                retVal = true;
                                closeDatabase();
                            }
                            else {
                                retVal = openDatabase(cData->newValue().toString());
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
                            retVal = true;
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
                            retVal = true;
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
                    retVal = true;
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
    return retVal;
}
}
