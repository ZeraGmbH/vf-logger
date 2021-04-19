#include "vl_databaselogger.h"
#include "vl_datasource.h"
#include "vl_databaseloggerprivate.h"

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

Q_LOGGING_CATEGORY(VEIN_LOGGER, VEIN_DEBUGNAME_LOGGER)

namespace VeinLogger
{

//constexpr definition, see: https://stackoverflow.com/questions/8016780/undefined-reference-to-static-constexpr-char

DatabaseLogger::DatabaseLogger(DataSource *t_dataSource, DBFactory t_factoryFunction, QObject *t_parent, AbstractLoggerDB::STORAGE_MODE t_storageMode) :
    VfCpp::VeinModuleEntity(3,t_parent),
    m_dPtr(new DataLoggerPrivate(this)),
    m_isInitilized(false)
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

    connect(this, &DatabaseLogger::sigAttached,this,&DatabaseLogger::initOnce);
    connect(this, &DatabaseLogger::sigAttached, [this](){ m_dPtr->initOnce(); });
    connect(&m_dPtr->m_batchedExecutionTimer, &QTimer::timeout, [this]() {
        m_dPtr->updateDBFileSizeInfo();
        if(m_dPtr->m_stateMachine.configuration().contains(m_dPtr->m_loggingDisabledState)) {
            m_dPtr->m_batchedExecutionTimer.stop();
        }
    });
    connect(&m_dPtr->m_schedulingTimer, &QTimer::timeout, [this]() {
        m_transactionList.clear();
        m_activeTransaction.setValue(m_transactionList.keys());
        setLoggingEnabled(false);
    });

    connect(&m_dPtr->m_countdownUpdateTimer, &QTimer::timeout, [this]() {
        m_dPtr->updateSchedulerCountdown();
    });


    // db error handling
    connect(this, &DatabaseLogger::sigDatabaseError, [this](const QString &t_errorString) {
        qCWarning(VEIN_LOGGER) << t_errorString;

        closeDatabase();
        m_dPtr->m_noUninitMessage = true;
        m_dPtr->setStatusText("Database error");
    });





}

DatabaseLogger::~DatabaseLogger()
{
    delete m_dPtr;
}

void DatabaseLogger::initOnce(){
    using namespace VfCpp;
    if(!m_isInitilized){
        createComponent("EntityName","_LoggingSystem",cVeinModuleComponent::Direction::constant);
        createComponent("ENUM_LogType",QVariantMap({{"snapshot", (int)LogType::snapshot},{"startStop", (int)LogType::startStop}, {"duration" , (int)LogType::duration}}));
        m_loggingStatus = createComponent("LoggingStatus","");

        //only activate logging in case this validator is ok;
        VeinLambdaValidator* enabledValidator=new VeinLambdaValidator([this](QVariant p_value)->QValidator::State{
                QValidator::State retVal=QValidator::State::Acceptable;
                if(!m_loggingEnabled.value()){
                if(m_sessionName.value().isEmpty()){
                retVal=QValidator::State::Invalid;
    }
                if(!m_databaseReady.value()){
                retVal=QValidator::State::Invalid;
    }
                if(m_scheduledLoggingEnabled.value()){
                retVal=QValidator::State::Invalid;
    }
    }
                return retVal;
    });
        m_loggingEnabled = createComponent("LoggingEnabled",false,cVeinModuleComponent::Direction::inOut,enabledValidator);
        m_activeTransaction = createComponent("AcitveTransactions",QStringList(),cVeinModuleComponent::Direction::out);
        m_databaseReady =  createComponent("DatabaseReady",false);
        m_databaseFile = createComponent("DatabaseFile","",cVeinModuleComponent::Direction::inOut,new QRegExpValidator(QRegExp("^([a-z]{0}|.*\.db)$")));
        connect(m_databaseFile.component().toStrongRef().data(),&VeinAbstractComponent::sigValueChanged,[this](QVariant p_value){
            if(!p_value.toString().isEmpty()){
                openDatabase(p_value);
            }else{
                closeDatabase();
            }
        });
        m_databaseErrorFile = createComponent("DatabaseErrorFile","",cVeinModuleComponent::Direction::inOut,new QRegExpValidator(QRegExp(".*")));
        m_databaseFileMimeType = createComponent("DatabaseFileMimeType","",cVeinModuleComponent::Direction::out);
        m_databaseFileSize = createComponent("DatabaseFileSize","",cVeinModuleComponent::Direction::out);
        m_filesystemInfo = createComponent("FilesystemInfo","",cVeinModuleComponent::Direction::out);
        m_filesystemFree = createComponent("FilesystemFree","",cVeinModuleComponent::Direction::out);
        m_filesystemTotal = createComponent("FilesystemTotal","",cVeinModuleComponent::Direction::out);
        m_scheduledLoggingEnabled = createComponent("ScheduledLoggingEnabled",false);
        m_scheduledLoggingCountdown = createComponent("ScheduledLoggingCountdown",0,cVeinModuleComponent::Direction::out);
        m_existingSessions = createComponent("ExistingSessions","",cVeinModuleComponent::Direction::out);
        m_customerData = createComponent("CustomerData","",cVeinModuleComponent::Direction::out);
        m_sessionName = createComponent("sessionName","",cVeinModuleComponent::Direction::inOut,new QRegExpValidator(QRegExp("^([aA-zZ]|[0-9]|[\\s]|[_,-,+,\\,\\/])*$")));
        connect(m_sessionName .component().toStrongRef().data(),&VeinAbstractComponent::sigValueChanged,this,&DatabaseLogger::openSession);
        m_availableContentSets = createComponent("availableContentSets",QStringList(),cVeinModuleComponent::Direction::out);
        m_sessionProxy = createProxyComponent(1,"Session","SessionProxy");
        connect(m_sessionProxy.component().toStrongRef().data(),&VeinAbstractComponent::sigValueChanged,[this](QVariant p_value){
            stopLogging(QList<int>());
        });

        m_availableContentSets=m_contentSetLoader.contentSetList(m_sessionProxy.value()).toList();
        connect(m_sessionProxy.component().toStrongRef().data(),&VeinAbstractComponent::sigValueChanged,[this](QVariant p_value){
            m_availableContentSets=m_contentSetLoader.contentSetList(p_value.toString()).toList();
        });


        createRpc(this,"RPC_stopLogging",VfCpp::cVeinModuleRpc::Param({{"p_transaction", "QList<int>"}}));
        createRpc(this,"RPC_startLogging",VfCpp::cVeinModuleRpc::Param({
                                                                           {"p_transaction" , "QString"},
                                                                           {"p_logType", "ENUM_LogType"},
                                                                           {"p_duration" , "float"},
                                                                           {"p_guiContext", "QString"},
                                                                           {"p_contentSets", "QStringList"}
                                                                       }));
        createRpc(this,"RPC_readTransaction",VfCpp::cVeinModuleRpc::Param({{"p_session", "QString"},{"p_transaction", "QString"}}));
        createRpc(this,"RPC_readSessionComponent",VfCpp::cVeinModuleRpc::Param({{"p_session", "QString"},{"p_entity", "QString"},{"p_component", "QString"}}));
        createRpc(this,"RPC_deleteSession",VfCpp::cVeinModuleRpc::Param({{"p_session", "QString"}}));
        m_isInitilized=true;
    }
}




bool DatabaseLogger::loggingEnabled() const
{
    return m_dPtr->m_stateMachine.configuration().contains(m_dPtr->m_loggingEnabledState);
}


bool DatabaseLogger::setContentSetPath(const QString &p_zeraContentSetPath, const QString &p_customerContentSetPath)
{
    return m_contentSetLoader.init(p_zeraContentSetPath,p_customerContentSetPath);

}

void DatabaseLogger::setLoggingEnabled(bool t_enabled)
{
        if(t_enabled) {
            emit sigLoggingStarted();
        }
        else {
            emit sigLoggingStopped();
        }
}

void DatabaseLogger::openDatabase(QVariant p_filePath)
{
    QString t_filePath=p_filePath.toString();
    m_dPtr->m_databaseFilePath = t_filePath;
    m_dPtr->m_noUninitMessage = false;
    // setup/init components
    QHash <QString, QVariant> fileInfoData;
    fileInfoData.insert(m_databaseErrorFile.name(), QString());
    fileInfoData.insert(m_databaseFile.name(), t_filePath);
    QString mimeInfo;
    qint64 fileSize = 0;
    // Mime & size are set (again) in database-ready - there we have a file definitely
    QFileInfo fileInfo(t_filePath);
    if(fileInfo.exists()) {
        fileSize = fileInfo.size();
        QMimeDatabase mimeDB;
        mimeInfo = mimeDB.mimeTypeForFile(fileInfo, QMimeDatabase::MatchContent).name();
    }
    fileInfoData.insert(m_databaseFileMimeType.name(), mimeInfo);
    fileInfoData.insert(m_databaseFileSize.name(), fileSize);
    m_databaseErrorFile=QString();
    m_databaseFile=t_filePath;
    m_databaseFileMimeType=mimeInfo;
    m_databaseFileSize=fileSize;
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
        connect(this, &DatabaseLogger::sigSingleShot, m_dPtr->m_database, &AbstractLoggerDB::runBatchedExecution);
        connect(m_dPtr->m_database, SIGNAL(sigNewSessionList(QStringList)), this, SLOT(updateSessionList(QStringList)));

        emit sigOpenDatabase(t_filePath);
    }
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

    setLoggingEnabled(false);
    m_existingSessions.setValue(QStringList());
    m_customerData=QString();
    m_sessionName=QString();


    m_dPtr->updateDBStorageInfo();

    //qCDebug(VEIN_LOGGER) << "Unloaded database:" << closedDb;
}

void DatabaseLogger::checkDatabaseStillValid()
{
    QFile dbFile(m_dPtr->m_databaseFilePath);
    if(!dbFile.exists()) {
        emit sigDatabaseError(QString("Watcher detected database file %1 is gone!").arg(m_dPtr->m_databaseFilePath));
    }
}


void DatabaseLogger::readSession(QString p_session)
{
    m_availableContentSets.setValue(m_contentSetLoader.contentSetList(p_session).toList());
}

void DatabaseLogger::openSession(QVariant p_session)
{
    QString sessionName=p_session.toString();
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
            m_customerData=m_dPtr->m_dataSource->getValue(200, "FileSelected").toString();
            emit sigAddSession(sessionName,tmpStaticData);
        }else{
             m_customerData=m_dPtr->m_database->readSessionComponent(sessionName,"CustomerData","FileSelected").toString();
        }
    }
}

/**
 * @brief DatabaseLogger::addScript
 * @param t_script
 * @todo Make customer data system optional
 */
QVariant DatabaseLogger::RPC_startLogging(QVariantMap p_parameters)
{
    // read vein parameters
    QString transactionName=p_parameters["p_transaction"].toString();
    LogType logType=(LogType)p_parameters["p_logType"].toInt();
    int duration=p_parameters["p_duration"].toInt();
    QString guiContext=p_parameters["p_guiContext"].toString();
    QStringList contentSets=p_parameters["p_contentSets"].toStringList();
    // set variable start values
    QVariant retVal=startLogging(transactionName,logType,duration,guiContext,contentSets);
    return retVal;
}

QVariant DatabaseLogger::RPC_stopLogging(QVariantMap p_parameters)
{

    QVariantList  transactionList=p_parameters["p_transaction"].toList();
    QList<int> transactions;
    for(QVariant ele: transactions){
        transactions.append(ele.toInt());
    }
    QVariant retVal=stopLogging(transactions);
    return retVal;
}


QVariant DatabaseLogger::RPC_deleteSession(QVariantMap p_parameters){
    QString session = p_parameters["p_session"].toString();
    QVariant retVal = deleteSession(session);
    return retVal;
}

QVariant DatabaseLogger::RPC_readSessionComponent(QVariantMap p_parameters){
    QVariant retVal;
    QString session = p_parameters["p_session"].toString();
    QString entity = p_parameters["p_entity"].toString();
    QString component = p_parameters["p_component"].toString();
    retVal=readSessionComponent(session,entity,component);
    return retVal;
}


QVariant DatabaseLogger::RPC_readTransaction(QVariantMap p_parameters){
    QString session = p_parameters["p_session"].toString();
    QString transaction = p_parameters["p_transaction"].toString();
    QJsonDocument retVal;
    if(m_dPtr->m_stateMachine.configuration().contains(m_dPtr->m_loggingDisabledState)){
        retVal=m_dPtr->m_database->readTransaction(transaction,session);
    }
    return QVariant::fromValue(retVal.toJson());
}

bool DatabaseLogger::deleteSession(QString p_session)
{
    bool retVal=false;
    retVal=m_dPtr->m_database->deleteSession(p_session);

    // check if deleted session is current Session and if it is set sessionName empty
    // We will not check retVal here. If something goes wrong and the session is still availabel the
    // user can choose it again without risking undefined behavior.
    if(p_session == m_sessionName.value()){
        m_sessionName = QString();
    }
    return retVal;
}

QJsonDocument DatabaseLogger::readTransaction(QString p_session, QString p_transaction)
{
    QJsonDocument retVal;
    if(m_dPtr->m_stateMachine.configuration().contains(m_dPtr->m_loggingDisabledState)){
        retVal=m_dPtr->m_database->readTransaction(p_transaction,p_session);
    }
    return retVal;
//    return QVariant::fromValue(retVal.toJson());
}

QVariant DatabaseLogger::readSessionComponent(QString p_session, QString p_entity, QString p_component)
{
    QVariant retVal;
    retVal=m_dPtr->m_database->readSessionComponent(p_session,p_entity,p_component);
    return retVal;
}

int DatabaseLogger::startLogging(QString p_transactionName, DatabaseLogger::LogType p_logType, int p_duration, QString p_guiContext, QStringList p_contentSets)
{
    int retVal=-1;
    QString sessionName=m_sessionName.value();



    // We will start logging if the Database is ready and a session and transaction name is set.
    // We could start multiple transactions at the same time, but this is prevented. The only additional transaction
    // possible while recording is running is a snapshot.
    // Attention! m_loggingEnabled is not the same as m_dPtr->m_loggingEnabledState because the status changes after the signal.
    // This rpc runs inside a thread. Therefore the signal is threaded and QT decides to do the statechange when ever it wants.
    if(!sessionName.isEmpty() && !p_transactionName.isEmpty()){
        if((m_dPtr->m_stateMachine.configuration().contains(m_dPtr->m_loggingDisabledState)) || (m_dPtr->m_loggingEnabledState)){
            if(m_loggingEnabled == false || p_logType == LogType::snapshot){

                //writes the values from the data source to the database, some values may never change so they need to be initialized
                QString tmpContentSets = p_contentSets.join(QLatin1Char(','));
                //add a new transaction and store ids in script.
                QVector<int> tmpTransactionIds = {m_dPtr->m_database->addTransaction(p_transactionName,sessionName, tmpContentSets, p_guiContext)};
                // add starttime to transaction. stop time is set in batch execution.
                m_dPtr->m_database->addStartTime(tmpTransactionIds.first(),QDateTime::currentDateTime());

                QMap<int, QStringList> tmpLoggedValues = readContentSets(p_contentSets);
                //        m_transactionList[tmpTransactionIds.first()]=tmpLoggedValues;

                for(const int tmpEntityId : tmpLoggedValues.uniqueKeys()) { //only process once for every entity
                    if(m_dPtr->m_dataSource->hasEntity(tmpEntityId)) {
                        if(m_dPtr->m_database->hasEntityId(tmpEntityId) == false) { // already in db?
                            emit sigAddEntity(tmpEntityId, m_dPtr->m_dataSource->getEntityName(tmpEntityId));
                        }
                        QStringList tmpComponents = tmpLoggedValues[tmpEntityId];
                        if(tmpComponents.size() == 0){
                            tmpComponents = m_dPtr->m_dataSource->getEntityComponentsForStore(tmpEntityId);
                        }
                        for(const QString &tmpComponentName : tmpComponents) {
                            QString componentToAdd = tmpComponentName;
                            if(m_dPtr->m_database->hasComponentName(componentToAdd) == false) {
                                emit sigAddComponent(componentToAdd);
                            }
                            // add initial values
                            emit sigAddLoggedValue(
                                        sessionName,
                                        tmpTransactionIds,
                                        tmpEntityId,
                                        componentToAdd,
                                        m_dPtr->m_dataSource->getValue(tmpEntityId, componentToAdd),
                                        QDateTime::currentDateTime());
                        }
                    }
                }
                if(p_logType == LogType::startStop || p_logType == LogType::duration){

                    m_transactionList.insert(tmpTransactionIds.first(),tmpLoggedValues);
                    m_activeTransaction.setValue(m_transactionList.keys());

                    if(p_logType == LogType::duration){
                        m_scheduledLoggingEnabled=true;
                        m_scheduledLoggingCountdown=p_duration;
                        m_dPtr->m_schedulingTimer.setInterval(p_duration);
                    }
                    setLoggingEnabled(true);
                }else{
                    emit sigSingleShot();
                }
                retVal=tmpTransactionIds.first();
            }
        }
    }
    return retVal;
}

bool DatabaseLogger::stopLogging(QList<int> p_transactions)
{
    bool retVal;
    if(p_transactions.size() > 0){
        for(int transaction : p_transactions){
            if(m_transactionList.contains(transaction)){
                m_transactionList.remove(transaction);
                retVal=true;
            }
        }
    }else{
        m_transactionList.clear();
    }
    if(m_transactionList.size()==0){
        setLoggingEnabled(false);
    }
    m_activeTransaction.setValue(m_transactionList.keys());
    return retVal;
}


bool DatabaseLogger::processEvent(QEvent *p_event)
{
    using namespace VeinEvent;
    using namespace VeinComponent;

    bool retVal =true;
    retVal=VfCpp::VeinModuleEntity::processEvent(p_event);
    if(m_dPtr->m_stateMachine.configuration().contains(m_dPtr->m_loggingEnabledState)){
        if(p_event->type()==CommandEvent::eventType()) {
            CommandEvent *cEvent = nullptr;
            EventData *evData = nullptr;
            cEvent = static_cast<CommandEvent *>(p_event);
            Q_ASSERT(cEvent != nullptr);

            evData = cEvent->eventData();
            Q_ASSERT(evData != nullptr);
            if(evData->type()==ComponentData::dataType()) {

                ComponentData *cData=nullptr;
                cData = static_cast<ComponentData *>(evData);

                if(cEvent->eventSubtype() == CommandEvent::EventSubtype::NOTIFICATION) {

                    Q_ASSERT(cData != nullptr);
                    QString sessionName = "";
                    QVector<int> transactionIds;
                    //check all scripts if they want to log the changed value

                    for(int transaction : m_transactionList.keys()){
                        if(m_transactionList[transaction].contains(evData->entityId())){
                                if(m_transactionList[transaction][evData->entityId()].size() == 0){
                                    transactionIds.append(transaction);
                                }
                                else if(m_transactionList[transaction][evData->entityId()].contains(cData->componentName())){
                                    transactionIds.append(transaction);
                                }
                        }
                    }
                    sessionName=m_sessionName.value();

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
        }
    }

    return retVal;
}



void DatabaseLogger::updateSessionList(QStringList p_sessions)
{
    m_existingSessions=p_sessions;
}

QMap<int,QStringList> DatabaseLogger::readContentSets(QStringList p_contentSets)
{
    QMap<int,QStringList> resultMap;
    for(auto contentSet : p_contentSets) {
        QMap<QString,QVector<QString>> map = m_contentSetLoader.readContentSet(contentSet);
        for(QString key: map.keys()){
            QSet<QString> tmpNew;
            QSet<QString> tmpOld;
            if(map[key].toList().size() != 0){
                tmpNew=QSet<QString>(map[key].toList().begin(),map[key].toList().end());
            }else{
                tmpNew=QSet<QString>();
            }
            if(resultMap[key.toInt()].size() != 0){
                tmpOld=QSet<QString>(resultMap[key.toInt()].begin(),resultMap[key.toInt()].end());
            }else{
                tmpOld=QSet<QString>();
            }

            tmpNew.unite(tmpOld);
            resultMap[key.toInt()]=tmpNew.values();
        }
    }
    return resultMap;
}

VfCpp::VeinSharedComp<bool> DatabaseLogger::DatabaseReady() const
{
    return m_databaseReady;
}

void DatabaseLogger::setDatabaseReady(const VfCpp::VeinSharedComp<bool> &DatabaseReady)
{
    m_databaseReady = DatabaseReady;
}

int DatabaseLogger::entityId() const
{
    return m_dPtr->m_entityId;
}

QString DatabaseLogger::entityName() const
{
    return m_dPtr->m_entityName;
}



}

