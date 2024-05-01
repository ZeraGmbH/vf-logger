#include "testloggerdb.h"
#include "testloggersystem.h"
#include <timemachineobject.h>
#include <QJsonArray>

TestLoggerDB* TestLoggerDB::m_instance = nullptr;

const QLatin1String TestLoggerDB::DBNameOpenOk = QLatin1String("/tmp/veindb-test/DB_NAME_OPEN_OK");
const QLatin1String TestLoggerDB::DBNameOpenErrorEarly = QLatin1String("DB_NAME_OPEN_ERR");
const QLatin1String TestLoggerDB::DBNameOpenErrorLate = QLatin1String("/tmp/DB_NAME_OPEN_ERR");

TestLoggerDB *TestLoggerDB::getInstance()
{
    return m_instance;
}

void TestLoggerDB::setCustomerDataAlreadyInDbSession(bool inSession)
{
    m_customerDataAlreadyInDbSession = inSession;
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

void TestLoggerDB::valuesFromNowOnAreRecorded()
{
    m_valuesAreInitial = false;
}

bool TestLoggerDB::hasEntityId(int entityId) const
{
    if(entityId == customerDataEntityId)
        return m_customerDataAlreadyInDbSession;
    return m_entitiesAdded.contains(entityId);
}

bool TestLoggerDB::hasComponentName(const QString &componentName) const
{
    if(CustomerDataSystem::getComponentNames().contains(componentName))
        return m_customerDataAlreadyInDbSession;
    return m_componentsAdded.contains(componentName);
}

bool TestLoggerDB::hasSessionName(const QString &sessionName) const
{
    return m_dbSessionNames.contains(sessionName);
}

void TestLoggerDB::setStorageMode(STORAGE_MODE storageMode)
{
    m_storageMode = storageMode;
}

void TestLoggerDB::initLocalData()
{
}

void TestLoggerDB::addComponent(const QString &componentName)
{
    m_componentsAdded.insert(componentName);
    emit m_testSignaller->sigComponentAdded(componentName);
}

void TestLoggerDB::addEntity(int entityId, QString entityName)
{
    m_entitiesAdded.insert(entityId, entityName);
    emit m_testSignaller->sigEntityAdded(entityId, entityName);
}

static const int testTransactionId = 42;

int TestLoggerDB::addTransaction(const QString &transactionName, const QString &sessionName, const QStringList &contentSets, const QString &guiContextName)
{
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

int TestLoggerDB::addSession(const QString &sessionName, QList<VeinLogger::DatabaseCommandInterface::ComponentInfo> componentsStoredOncePerSession)
{
    // for vf-logger
    m_dbSessionNames.append(sessionName);
    emit sigNewSessionList(m_dbSessionNames);

    // for test
    m_valueWriteCount++;

    QMap<QString, VeinLogger::DatabaseCommandInterface::ComponentInfo> componentValuesSorted;
    for(int i=0; i<componentsStoredOncePerSession.count(); i++) {
        VeinLogger::DatabaseCommandInterface::ComponentInfo variantEntry = componentsStoredOncePerSession[i];
        Q_ASSERT(variantEntry.timestamp.isValid());
        // timestamps must be there but are not suited for textfile dumps
        variantEntry.timestamp = QDateTime::fromSecsSinceEpoch(m_valueWriteCount, Qt::UTC);

        Q_ASSERT(!variantEntry.componentName.isEmpty());
        componentValuesSorted.insert(variantEntry.componentName, variantEntry);
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

bool TestLoggerDB::deleteSession(const QString &session)
{

}

void TestLoggerDB::addLoggedValue(const QString &sessionName, QVector<int> transactionIds, VeinLogger::DatabaseCommandInterface::ComponentInfo component)
{
    if(!transactionIds.contains(testTransactionId))
        qFatal("Unexpected transaction ids!");

    m_valueWriteCount++;

    if(m_valuesAreInitial) {
        InitialValue initVal = { sessionName, component.value };
        m_initialValues[component.entityId][component.componentName] = initVal;
    }
    else {
        LoggedValue logVal = { sessionName, component.entityId, component.componentName, component.value, m_valueWriteCount};
        m_loggedValues.append(logVal);
    }
}

bool TestLoggerDB::openDatabase(const QString &dbPath)
{
    // This one seems to run in another thread so we can use emit
    if(dbPath == DBNameOpenOk) {
        QFile fileForWatcher(dbPath);
        fileForWatcher.open(QIODevice::WriteOnly);
        fileForWatcher.close();
        m_openDbPath = dbPath;
        emit sigNewSessionList(m_dbSessionNames);
        TimeMachineObject::feedEventLoop();

        emit sigDatabaseReady();
        TimeMachineObject::feedEventLoop();
    }
    else {
        m_openDbPath.clear();
        emit sigDatabaseError("Could not open test database");
    }
    return !m_openDbPath.isEmpty();
}

void TestLoggerDB::runBatchedExecution()
{
    // Surprise: This is a poor mimic of what SQLite implementation does
    addStopTime(testTransactionId, QDateTime());
}
