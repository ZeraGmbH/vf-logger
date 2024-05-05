#ifndef DATALOGGERPRIVATE_H
#define DATALOGGERPRIVATE_H

#include "vl_databaselogger.h"
#include <vf-cpp-rpc.h>

class DataLoggerPrivate: public QObject
{
public:
    explicit DataLoggerPrivate(VeinLogger::DatabaseLogger *qPtr);
    ~DataLoggerPrivate();

    void initOnce();

    bool m_initDone=false;

    QMap<QString, VfCpp::cVeinModuleRpc::Ptr> m_rpcList;

    //component names

    VeinLogger::DatabaseLogger *m_qPtr = nullptr;
    friend class DatabaseLogger;
};

#endif // DATALOGGERPRIVATE_H
