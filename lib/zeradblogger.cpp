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

class ZeraDBLoggerPrivate
{

    enum RPCResultCodes {
        RPC_CANCELED = -64,
        RPC_EINVAL = -EINVAL, //invalid parameters
        RPC_SUCCESS = 0
    };

    ZeraDBLoggerPrivate(ZeraDBLogger *t_qPtr) :
        m_qPtr(t_qPtr)
    {

    }

    ZeraDBLogger *m_qPtr=nullptr;

    friend class ZeraDBLogger;
};

ZeraDBLogger::ZeraDBLogger(VeinLogger::DataSource *t_dataSource, VeinLogger::DBFactory t_factoryFunction, QObject *parent) :
    VeinLogger::DatabaseLogger(t_dataSource, t_factoryFunction, parent),
    m_dPtr(new ZeraDBLoggerPrivate(this))
{
}

ZeraDBLogger::~ZeraDBLogger()
{
    delete m_dPtr;
}

void ZeraDBLogger::processEvent(QEvent *t_event)
{
    VeinLogger::DatabaseLogger::processEvent(t_event);
}

