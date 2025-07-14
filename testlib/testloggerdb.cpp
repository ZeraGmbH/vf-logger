#include "testloggerdb.h"
#include "testloggersystem.h"
#include "jsonloggedvalues.h"
#include <timemachineobject.h>
#include <QJsonArray>
#include <qfileinfo.h>

TestLoggerDB* TestLoggerDB::m_instance = nullptr;

TestLoggerDB *TestLoggerDB::getCurrentInstance()
{
    return m_instance;
}

TestLoggerDB::TestLoggerDB(TestDbAddSignaller *testSignaller) :
    m_testSignaller(testSignaller)
{
    if(m_instance)
        qFatal("m_instance is set!");
    m_instance = this;
}

TestLoggerDB::~TestLoggerDB()
{
    deleteDbFile();
    if(m_instance != this)
        qFatal("Seems another instance was created!");
    m_instance = nullptr;
}

void TestLoggerDB::deleteDbFile()
{
    if(!m_openDbPath.isEmpty()) {
        QFile fileForWatcher(m_openDbPath);
        fileForWatcher.remove();
    }
}

QByteArray TestLoggerDB::getJsonDumpedComponentStored()
{
    QJsonObject initialData;
    for(auto entityIter=m_initialValues.cbegin(); entityIter!=m_initialValues.cend(); entityIter++) {
        const int entityId = entityIter.key();
        const QMap<QString, InitialValue> &component = entityIter.value();
        QJsonObject entityValues;
        for(auto componentIter=component.cbegin(); componentIter!=component.cend(); componentIter++) {
            QString componentName = componentIter.key();
            InitialValue value = componentIter.value();
            QJsonObject componentValues;
            componentValues.insert("sessionName", value.sessionName);
            componentValues.insert("value", value.value.toString());
            entityValues.insert(componentName, componentValues);
        }
        initialData.insert(QString::number(entityId), entityValues);
    }

    QJsonArray recordedData;
    for(const auto &loggedVal : qAsConst(m_loggedValues)) {
        QJsonObject entry;
        entry.insert("sessionName", loggedVal.sessionName);
        entry.insert("entityId", loggedVal.entityId);
        entry.insert("componentName", loggedVal.componentName);
        entry.insert("entity_component_value", loggedVal.value.toJsonValue());
        entry.insert("writeId", loggedVal.dataWriteIdCount);
        recordedData.append(entry);
    }

    QJsonObject loggedValues;
    loggedValues.insert("AddedStartStop", m_startStopEvents);
    loggedValues.insert("AddSessionValues", m_sessionOnceComponentsAdded);
    loggedValues.insert("OnLoggerStartValues", initialData);
    loggedValues.insert("ValuesRecordedChronological", recordedData);
    return QJsonDocument(loggedValues).toJson();
}

void TestLoggerDB::valuesFromNowOnAreInitial()
{
    m_valuesAreInitial = true;
}

void TestLoggerDB::valuesFromNowOnAreRecorded()
{
    m_valuesAreInitial = false;
}

bool TestLoggerDB::hasSessionName(const QString &sessionName) const
{
    return m_dbSessionNames.contains(sessionName);
}

void TestLoggerDB::setStorageMode(STORAGE_MODE storageMode)
{
    m_storageMode = storageMode;
}

static const int testTransactionId = 42;

int TestLoggerDB::addTransaction(const QString &transactionName, const QString &sessionName, const QStringList &contentSets, const QString &guiContextName)
{
    Transactions currentTransacions = m_sessions.value(sessionName);
    currentTransacions.insert(transactionName, TransactionInfo{guiContextName, contentSets});
    m_sessions[sessionName] = currentTransacions;
    emit m_testSignaller->sigAddTransaction(transactionName, sessionName, contentSets, guiContextName);
    return testTransactionId;
}

bool TestLoggerDB::addStartTime(int transactionId, QDateTime time)
{
    Q_UNUSED(time);
    if(transactionId != testTransactionId)
        qFatal("Unexpected transaction id: %i!", transactionId);
    QJsonObject entry {{"StartTime", QJsonValue(m_valueWriteCount)}};
    m_startStopEvents.append(entry);
    return true;
}

bool TestLoggerDB::addStopTime(int transactionId, QDateTime time)
{
    Q_UNUSED(time);
    if(transactionId != testTransactionId)
        qFatal("Unexpected transaction id: %i!", transactionId);
    QJsonObject entry {{"StopTime", QJsonValue(m_valueWriteCount)}};
    m_startStopEvents.append(entry);
    return true;
}

QVariant TestLoggerDB::readSessionComponent(const QString &p_session, const QString &p_entity, const QString &p_component)
{
    if(p_entity == "CustomerData" && p_component == "FileSelected")
        return TestLoggerSystem::getCustomerDataPath() + "test_customer_data.json";
}

void TestLoggerDB::onDisplaySessionsInfos(QUuid callId, const QString &sessionName)
{
    if(!m_sessions.contains(sessionName))
        emit sigDisplaySessionInfosCompleted(callId, false, "Select an existing session", QJsonObject());
    else {
        QJsonObject sessionObject;
        Transactions allTransactions = m_sessions.value(sessionName);
        for(auto transactionName: allTransactions.keys()) {
            QJsonObject transactionObject;
            transactionObject.insert("contentset", allTransactions.value(transactionName).contentSetList.join(","));
            transactionObject.insert("guicontext", allTransactions.value(transactionName).guiContext);
            sessionObject.insert(transactionName, transactionObject);
        }
        emit sigDisplaySessionInfosCompleted(callId, true, QString(), sessionObject);
    }
}

void TestLoggerDB::onDeleteTransaction(QUuid callId, const QString &transactionName)
{
    bool deleted = false;
    for(QString sessionName: m_sessions.keys()) {
        if(m_sessions[sessionName].contains(transactionName)) {
            m_sessions[sessionName].remove(transactionName);
            deleted = true;
        }
    }
    if(deleted)
        emit sigDeleteTransactionCompleted(callId, true, QString());
    else
        emit sigDeleteTransactionCompleted(callId, false, "Select an existing transaction");
}

void TestLoggerDB::onListAllSessions(QUuid callId)
{
    QJsonArray allSessions;
    for(int id = 0; id < m_sessions.count(); id++) {
        QJsonObject sessionJson;
        sessionJson["id"] = id+1;
        sessionJson["sessionName"] = m_sessions.keys().at(id);
        allSessions.append(sessionJson);
    }
    emit sigListAllSessionsCompleted(callId, true, QString(), allSessions);
}

void TestLoggerDB::onDisplayActualValues(QUuid callId, const QString &transactionName)
{
    bool transactionFound = false;
    QStringList contentsetList;
    for(QString sessionName: m_sessions.keys()) {
        if(m_sessions[sessionName].contains(transactionName)) {
            Transactions transaction = m_sessions[sessionName];
            TransactionInfo transactionInfo = transaction[transactionName];
            contentsetList = transactionInfo.contentSetList;
            transactionFound = true;
        }
    }
    JsonLoggedValues jsonLoggedValues(contentsetList);
    QString sessionDeviceName = "";
    for(int entityId : m_initialValues.keys()) {
        QMap<QString, InitialValue> compoValuesMap = m_initialValues[entityId];
        for(QString componentName : compoValuesMap.keys()) {
            InitialValue initValue = compoValuesMap[componentName];
            QVariant value = initValue.value;
            jsonLoggedValues.appendLoggedValues(QString::number(entityId), componentName, value.toJsonValue());
            if(entityId == 0 && componentName == "Session")
                sessionDeviceName = value.toString();
        }
    }
    if(transactionFound)
        emit sigDisplayActualValuesCompleted(callId, true, QString(), jsonLoggedValues.createLoggedValuesJson(sessionDeviceName));
    else
        emit sigDisplayActualValuesCompleted(callId, false, "Select an existing transaction", QJsonObject());
}

int TestLoggerDB::addSession(const QString &sessionName, QList<VeinLogger::DatabaseCommandInterface::ComponentInfo> componentsStoredOncePerSession)
{
    // for vf-logger
    m_dbSessionNames.append(sessionName);
    emit sigNewSessionList(m_dbSessionNames);
    m_sessions.insert(sessionName, Transactions());

    QMap<QString, VeinLogger::DatabaseCommandInterface::ComponentInfo> componentValuesSorted;
    for(int i=0; i<componentsStoredOncePerSession.count(); i++) {
        VeinLogger::DatabaseCommandInterface::ComponentInfo variantEntry = componentsStoredOncePerSession[i];
        Q_ASSERT(variantEntry.timestamp.isValid());
        // timestamps must be there but are not suited for textfile dumps
        variantEntry.timestamp = QDateTime::fromSecsSinceEpoch(m_valueWriteCount, Qt::UTC);

        Q_ASSERT(!variantEntry.componentName.isEmpty());
        componentValuesSorted.insert(variantEntry.componentName, variantEntry);

        emit m_testSignaller->sigEntityAdded(variantEntry.entityId, variantEntry.entityName);
        emit m_testSignaller->sigComponentAdded(variantEntry.componentName);
    }
    QJsonArray jsonArray;
    for(const auto &component : componentValuesSorted) {
        QJsonObject entry;
        entry["entityId"] = component.entityId;
        entry["entityName"] = component.entityName;
        entry["componentName"] = component.componentName;
        entry["value"] = component.value.toJsonValue();
        entry["testtime"] = component.timestamp.toSecsSinceEpoch();
        jsonArray.append(entry);
    }
    QJsonObject staticJson;
    staticJson.insert(sessionName, jsonArray);
    m_sessionOnceComponentsAdded.append(staticJson);

    return m_dbSessionNames.count();
}

void TestLoggerDB::onDeleteSession(QUuid callId, const QString &session)
{
    if(!m_sessions.contains(session))
        emit sigDeleteSessionCompleted(callId, false, "Select an existing session", QStringList());
    else{
        m_sessions.remove(session);
        m_dbSessionNames.removeOne(session);
        emit sigDeleteSessionCompleted(callId, true, QString(), m_dbSessionNames);
    }
}

void TestLoggerDB::addLoggedValue(const QString &sessionName, QVector<int> transactionIds, VeinLogger::DatabaseCommandInterface::ComponentInfo component)
{
    if(!transactionIds.contains(testTransactionId))
        qFatal("Unexpected transaction ids!");

    if(m_valuesAreInitial) {
        InitialValue initVal = { sessionName, component.value };
        m_initialValues[component.entityId][component.componentName] = initVal;

        emit m_testSignaller->sigEntityAdded(component.entityId, component.entityName);
        emit m_testSignaller->sigComponentAdded(component.componentName);
    }
    else {
        LoggedValue logVal = { sessionName, component.entityId, component.componentName, component.value, m_valueWriteCount};
        m_loggedValues.append(logVal);
    }
    m_valueWriteCount++;
}

void TestLoggerDB::setNextValueWriteCount(int newValueWriteCount)
{
    m_valueWriteCount = newValueWriteCount;
}

void TestLoggerDB::onOpen(const QString &dbPath)
{
    // This one seems to run in another thread so we can use emit
    QFileInfo fInfo(dbPath);
    if(fInfo.exists() && !fInfo.isWritable()){
        emit sigDatabaseError(QString("Database is read only"));
        return;
    }
    QFile fileForWatcher(dbPath);
    fileForWatcher.open(QIODevice::WriteOnly);
    fileForWatcher.close();
    m_openDbPath = dbPath;
    emit sigNewSessionList(m_dbSessionNames);
    for(auto session: m_dbSessionNames)
        m_sessions.insert(session, Transactions());
    TimeMachineObject::feedEventLoop();

    emit sigDatabaseReady();
    TimeMachineObject::feedEventLoop();

}

void TestLoggerDB::runBatchedExecution()
{
    // Surprise: This is a poor mimic of what SQLite implementation does
    addStopTime(testTransactionId, QDateTime());
}
