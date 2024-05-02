#include "databasecommandinterface.h"
#include "vl_abstractloggerdb.h"

namespace VeinLogger
{
void DatabaseCommandInterface::connectDb(AbstractLoggerDB *db)
{
    connect(this, &DatabaseCommandInterface::sigAddLoggedValue, db, &AbstractLoggerDB::addLoggedValue);
    connect(this, &DatabaseCommandInterface::sigAddSession, db, &AbstractLoggerDB::addSession);
    connect(this, &DatabaseCommandInterface::sigOpenDatabase, db, &AbstractLoggerDB::openDatabase);
}
}
