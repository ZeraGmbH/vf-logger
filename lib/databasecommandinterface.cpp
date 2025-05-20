#include "databasecommandinterface.h"
#include "vl_abstractloggerdb.h"

Q_DECLARE_METATYPE(VeinLogger::DatabaseCommandInterface::ComponentInfo)

namespace VeinLogger
{

bool DatabaseCommandInterface::m_componentInfoMetaWasRegistered = false;

DatabaseCommandInterface::DatabaseCommandInterface()
{
    if(!m_componentInfoMetaWasRegistered) {
        qRegisterMetaType<QVector<int>>();
        qRegisterMetaType<ComponentInfo>();
        qRegisterMetaType<QList<VeinLogger::DatabaseCommandInterface::ComponentInfo>>();
        m_componentInfoMetaWasRegistered = true;
    }
}

void DatabaseCommandInterface::connectDb(AbstractLoggerDB *db)
{
    m_databaseConnected = true;
    connect(this, &DatabaseCommandInterface::sigAddTransaction, db, &AbstractLoggerDB::onAddTransaction, Qt::QueuedConnection);
    connect(this, &DatabaseCommandInterface::sigAddStartTime, db, &AbstractLoggerDB::onAddStartTime, Qt::QueuedConnection);
    connect(this, &DatabaseCommandInterface::sigAddLoggedValue, db, &AbstractLoggerDB::addLoggedValue, Qt::QueuedConnection);
    connect(this, &DatabaseCommandInterface::sigAddSession, db, &AbstractLoggerDB::addSession, Qt::QueuedConnection);
    connect(this, &DatabaseCommandInterface::sigOpenDatabase, db, &AbstractLoggerDB::onOpen, Qt::QueuedConnection);
    connect(this, &DatabaseCommandInterface::sigDeleteSession, db, &AbstractLoggerDB::onDeleteSession, Qt::QueuedConnection);
    connect(this, &DatabaseCommandInterface::sigDeleteTransaction, db, &AbstractLoggerDB::onDeleteTransaction, Qt::QueuedConnection);
    connect(this, &DatabaseCommandInterface::sigDisplaySessionInfos, db, &AbstractLoggerDB::onDisplaySessionsInfos, Qt::QueuedConnection);
    connect(this, &DatabaseCommandInterface::sigListAllSessions, db, &AbstractLoggerDB::onListAllSessions, Qt::QueuedConnection);
    connect(this, &DatabaseCommandInterface::sigDisplayActualValues, db, &AbstractLoggerDB::onDisplayActualValues, Qt::QueuedConnection);
    connect(this, &DatabaseCommandInterface::sigFlushToDb, db, &AbstractLoggerDB::runBatchedExecution, Qt::QueuedConnection);
}

bool DatabaseCommandInterface::isDatabaseConnected()
{
    return m_databaseConnected;
}
}
