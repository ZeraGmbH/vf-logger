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
    connect(this, &DatabaseCommandInterface::sigAddLoggedValue, db, &AbstractLoggerDB::addLoggedValue);
    connect(this, &DatabaseCommandInterface::sigAddSession, db, &AbstractLoggerDB::addSession);
    connect(this, &DatabaseCommandInterface::sigOpenDatabase, db, &AbstractLoggerDB::openDatabase);
}
}
