#include "testloggerdb.h"
#include "testloggersystem.h"
#include <timemachineobject.h>
#include <QJsonArray>

TestLoggerDB* TestLoggerDB::m_instance = nullptr;
bool TestLoggerDB::m_customerDataSupported = false;

const QLatin1String TestLoggerDB::DBNameOpenOk = QLatin1String("/tmp/DB_NAME_OPEN_OK");
const QLatin1String TestLoggerDB::DBNameOpenErrorEarly = QLatin1String("DB_NAME_OPEN_ERR");
const QLatin1String TestLoggerDB::DBNameOpenErrorLate = QLatin1String("/tmp/DB_NAME_OPEN_ERR");

TestLoggerDB *TestLoggerDB::getInstance()
{
    return m_instance;
}

void TestLoggerDB::setCustomerDataSupported(bool supported)
{
    m_customerDataSupported = supported;
}

TestLoggerDB::TestLoggerDB()
{
    Q_ASSERT(!m_instance);
    m_instance = this;
}

TestLoggerDB::~TestLoggerDB()
{
    deleteDbFile();
    Q_ASSERT(m_instance == this);
    m_instance = nullptr;
}

void TestLoggerDB::deleteDbFile()
{
    if(!m_openDbPath.isEmpty()) {
        QFile fileForWatcher(m_openDbPath);
        fileForWatcher.remove();
    }
}

bool TestLoggerDB::hasEntityId(int entityId) const
{
    if(entityId == customerDataEntityId)
        return m_customerDataSupported;
}

bool TestLoggerDB::hasComponentName(const QString &componentName) const
{
    if(CustomerDataSystem::getComponentNames().contains(componentName))
        return m_customerDataSupported;
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

std::function<bool (QString)> TestLoggerDB::getDatabaseValidationFunction() const
{

}

void TestLoggerDB::initLocalData()
{

}

void TestLoggerDB::addComponent(const QString &componentName)
{

}

void TestLoggerDB::addEntity(int entityId, QString entityName)
{

}

int TestLoggerDB::addTransaction(const QString &transactionName, const QString &sessionName, const QString &contentSets, const QString &guiContextName)
{

}

bool TestLoggerDB::addStartTime(int transactionId, QDateTime time)
{

}

bool TestLoggerDB::addStopTime(int transactionId, QDateTime time)
{

}

QJsonDocument TestLoggerDB::readTransaction(const QString &p_transaction, const QString &p_session)
{

}

QVariant TestLoggerDB::readSessionComponent(const QString &p_session, const QString &p_entity, const QString &p_component)
{
    if(p_entity == "CustomerData" && p_component == "FileSelected")
        return TestLoggerSystem::getCustomerDataPath() + "test_customer_data.json";
}

int TestLoggerDB::addSession(const QString &sessionName, QList<QVariantMap> staticData)
{
    // for vf-logger
    m_dbSessionNames.append(sessionName);
    emit sigNewSessionList(m_dbSessionNames);

    // for test sequence regression
    m_dataWriteIdCount++;
    QJsonObject jsonData;
    for(int i=0; i<staticData.count(); i++) {
        QVariantMap variantEntry = staticData[i];
        Q_ASSERT(variantEntry.contains("time"));
        // timestamps must be there but are not suited for textfile dumps
        variantEntry["time"] = QDateTime::fromSecsSinceEpoch(m_dataWriteIdCount, Qt::UTC);

        // Ensure sorting
        QJsonObject jsonEntry = QJsonObject::fromVariantMap(variantEntry);
        Q_ASSERT(jsonEntry.contains("compName"));
        jsonData.insert(jsonEntry["compName"].toString(), jsonEntry);
    }
    QJsonObject staticJson;
    staticJson.insert(sessionName, jsonData);
    emit sigSessionStaticDataAdded(staticJson);

    return m_dbSessionNames.count();
}

bool TestLoggerDB::deleteSession(const QString &session)
{

}

void TestLoggerDB::addLoggedValue(const QString &sessionName, QVector<int> transactionIds, int entityId, const QString &componentName, QVariant value, QDateTime timestamp)
{

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
