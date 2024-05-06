#include "dataloggerprivate.h"
#include "loggerstatictexts.h"
#include <vcmp_entitydata.h>
#include <vf_server_component_setter.h>
#include <QFileInfo>

using namespace VeinLogger;

DataLoggerPrivate::DataLoggerPrivate(DatabaseLogger *qPtr) : m_qPtr(qPtr)
{
}

DataLoggerPrivate::~DataLoggerPrivate()
{
}

