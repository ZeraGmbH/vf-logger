#include "testloggerdb.h"
#include <timemachineobject.h>

const QLatin1String TestLoggerDB::DBNameOpenOk = QLatin1String("/tmp/DB_NAME_OPEN_OK");
const QLatin1String TestLoggerDB::DBNameOpenError = QLatin1String("DB_NAME_OPEN_ERR");

bool TestLoggerDB::hasEntityId(int entityId) const
{

}

bool TestLoggerDB::hasComponentName(const QString &componentName) const
{

}

bool TestLoggerDB::hasSessionName(const QString &sessionName) const
{

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
        emit sigNewSessionList(QStringList() << "TestSession1" << "TestSession2");
        /*QMetaObject::invokeMethod(this,
                                  "sigNewSessionList",
                                  Qt::QueuedConnection,
                                  Q_ARG(QStringList, QStringList() << "TestSession1" << "TestSession2"));*/
        TimeMachineObject::feedEventLoop();

        emit sigDatabaseReady();
        /*QMetaObject::invokeMethod(this,
                                  "sigDatabaseReady",
                                  Qt::QueuedConnection);*/
        TimeMachineObject::feedEventLoop();
    }
    else
        emit sigDatabaseError("Could not open test database");
        /*QMetaObject::invokeMethod(this,
                                  "sigDatabaseError",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, "Could not open test database"));*/
    return !m_openDbPath.isEmpty();
}

void TestLoggerDB::runBatchedExecution()
{

}
