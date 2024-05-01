#include "databasecommandinterface.h"


void VeinLogger::DatabaseCommandInterface::connectDb(AbstractLoggerDB *db)
{
    connect(this, &DatabaseCommandInterface::sigAddLoggedValue, db, &AbstractLoggerDB::addLoggedValue);
    connect(this, &DatabaseCommandInterface::sigAddEntity, db, &AbstractLoggerDB::addEntity);
    connect(this, &DatabaseCommandInterface::sigAddComponent, db, &AbstractLoggerDB::addComponent);
    connect(this, &DatabaseCommandInterface::sigAddSession, db, &AbstractLoggerDB::addSession);
    connect(this, &DatabaseCommandInterface::sigOpenDatabase, db, &AbstractLoggerDB::openDatabase);
}
