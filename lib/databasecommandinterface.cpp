#include "databasecommandinterface.h"
#include "vl_abstractloggerdb.h"

Q_DECLARE_METATYPE(VeinLogger::DatabaseCommandInterface::ComponentInfo)

namespace VeinLogger
{

bool DatabaseCommandInterface::m_componentInfoWasRegistered = false;

DatabaseCommandInterface::DatabaseCommandInterface()
{
    if(!m_componentInfoWasRegistered) {
        qRegisterMetaType<ComponentInfo>();
        m_componentInfoWasRegistered = true;
    }
}

void DatabaseCommandInterface::connectDb(AbstractLoggerDB *db)
{
    connect(this, &DatabaseCommandInterface::sigAddLoggedValue, db, &AbstractLoggerDB::addLoggedValue);
    connect(this, &DatabaseCommandInterface::sigAddSession, db, &AbstractLoggerDB::addSession);
    connect(this, &DatabaseCommandInterface::sigOpenDatabase, db, &AbstractLoggerDB::openDatabase);
}
}
