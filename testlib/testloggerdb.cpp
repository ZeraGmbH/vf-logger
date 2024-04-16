#include "testloggerdb.h"
#include <timemachineobject.h>
#include <QJsonArray>

TestLoggerDB* TestLoggerDB::m_instance = nullptr;

const QLatin1String TestLoggerDB::DBNameOpenOk = QLatin1String("/tmp/DB_NAME_OPEN_OK");
const QLatin1String TestLoggerDB::DBNameOpenErrorEarly = QLatin1String("DB_NAME_OPEN_ERR");
const QLatin1String TestLoggerDB::DBNameOpenErrorLate = QLatin1String("/tmp/DB_NAME_OPEN_ERR");

TestLoggerDB *TestLoggerDB::getInstance()
{
    return m_instance;
}

TestLoggerDB::TestLoggerDB()
{
    Q_ASSERT(!m_instance);
    m_instance = this;
}

TestLoggerDB::~TestLoggerDB()
{
    Q_ASSERT(m_instance == this);
    m_instance = nullptr;
}

bool TestLoggerDB::hasEntityId(int entityId) const
{

}

bool TestLoggerDB::hasComponentName(const QString &componentName) const
{

}

bool TestLoggerDB::hasSessionName(const QString &sessionName) const
{
    return m_sessionNames.contains(sessionName);
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

}

int TestLoggerDB::addSession(const QString &sessionName, QList<QVariantMap> staticData)
{
    // for vf-logger
    m_sessionNames.append(sessionName);
    emit sigNewSessionList(m_sessionNames);

    // for test
    QJsonArray array;
    for(int i=0; i<staticData.count(); i++) {
        QJsonObject entry = QJsonObject::fromVariantMap(staticData[i]);
        array.append(entry);
    }
    QJsonObject staticJson;
    staticJson.insert(sessionName, array);
    emit sigSessionAdded(staticJson);

    return m_sessionNames.count();
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
        m_openDbPath = dbPath;
        emit sigNewSessionList(m_sessionNames);
        TimeMachineObject::feedEventLoop();

        emit sigDatabaseReady();
        TimeMachineObject::feedEventLoop();
    }
    else
        emit sigDatabaseError("Could not open test database");
    return !m_openDbPath.isEmpty();
}

void TestLoggerDB::runBatchedExecution()
{

}
