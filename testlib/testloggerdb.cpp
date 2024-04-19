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

TestLoggerDB::TestLoggerDB()
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

bool TestLoggerDB::databaseIsOpen() const
{
    return !m_openDbPath.isEmpty();
}

QString TestLoggerDB::databasePath() const
{
    return m_openDbPath;
}

void TestLoggerDB::setStorageMode(STORAGE_MODE storageMode)
{
    m_storageMode = storageMode;
}

VeinLogger::AbstractLoggerDB::STORAGE_MODE TestLoggerDB::getStorageMode() const
{
    return m_storageMode;
}

void TestLoggerDB::initLocalData()
{
}

void TestLoggerDB::addComponent(const QString &componentName)
{
    m_componentsAdded.insert(componentName);
    emit sigComponentAdded(componentName);
}

void TestLoggerDB::addEntity(int entityId, QString entityName)
{
    m_entitiesAdded.insert(entityId, entityName);
    emit sigEntityAdded(entityId, entityName);
}

static const int testTransactionId = 42;

int TestLoggerDB::addTransaction(const QString &transactionName, const QString &sessionName, const QString &contentSets, const QString &guiContextName)
{
    return testTransactionId;
}

bool TestLoggerDB::addStartTime(int transactionId, QDateTime time)
{
    Q_UNUSED(time);
    if(transactionId != testTransactionId)
        qFatal("Unexpected transaction id: %i!", transactionId);
    return true;
}

bool TestLoggerDB::addStopTime(int transactionId, QDateTime time)
{
    Q_UNUSED(time);
    if(transactionId != testTransactionId)
        qFatal("Unexpected transaction id: %i!", transactionId);
    return true;
}

QVariant TestLoggerDB::readSessionComponent(const QString &p_session, const QString &p_entity, const QString &p_component)
{
    if(p_entity == "CustomerData" && p_component == "FileSelected")
        return TestLoggerSystem::getCustomerDataPath() + "test_customer_data.json";
}

int TestLoggerDB::addSession(const QString &sessionName, QList<QVariantMap> componentValuesStoredOncePerSession)
{
    // for vf-logger
    m_dbSessionNames.append(sessionName);
    emit sigNewSessionList(m_dbSessionNames);

    // for test
    m_valueWriteCount++;

    QMap<QString, QVariantMap> componentValuesSorted;
    for(int i=0; i<componentValuesStoredOncePerSession.count(); i++) {
        QVariantMap variantEntry = componentValuesStoredOncePerSession[i];
        Q_ASSERT(variantEntry.contains("time"));
        // timestamps must be there but are not suited for textfile dumps
        variantEntry["time"] = QDateTime::fromSecsSinceEpoch(m_valueWriteCount, Qt::UTC);

        Q_ASSERT(variantEntry.contains("compName"));
        componentValuesSorted.insert(variantEntry["compName"].toString(), variantEntry);
    }
    QJsonArray jsonArray;
    for(const QVariantMap &variantEntry : componentValuesSorted) {
        jsonArray.append(QJsonObject::fromVariantMap(variantEntry));
    }
    QJsonObject staticJson;
    staticJson.insert(sessionName, jsonArray);
    m_sessionOnceComponentsAdded.append(staticJson);

    return m_dbSessionNames.count();
}

bool TestLoggerDB::deleteSession(const QString &session)
{

}

void TestLoggerDB::addLoggedValue(const QString &sessionName, QVector<int> transactionIds, int entityId, const QString &componentName, QVariant value, QDateTime timestamp)
{
    Q_UNUSED(timestamp)
    if(!transactionIds.contains(testTransactionId))
        qFatal("Unexpected transaction ids!");

    m_valueWriteCount++;

    if(m_valuesAreInitial) {
        InitialValue initVal = { sessionName, value };
        m_initialValues[entityId][componentName] = initVal;
    }
    else {
        LoggedValue logVal = { sessionName, entityId, componentName, value, m_valueWriteCount};
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

}
