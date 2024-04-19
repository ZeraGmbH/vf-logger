#include "zeradblogger.h"

#include <vcmp_componentdata.h>
#include <vcmp_remoteproceduredata.h>
#include <vcmp_errordata.h>
#include <ve_commandevent.h>

#include <QStorageInfo>
#include <QDir>
#include <QDirIterator>
#include <QThread>

#include <functional>

ZeraDBLogger::ZeraDBLogger(VeinLogger::DataSource *t_dataSource, VeinLogger::DBFactory t_factoryFunction, QObject *parent) :
    VeinLogger::DatabaseLogger(t_dataSource, t_factoryFunction, parent)
{
}
