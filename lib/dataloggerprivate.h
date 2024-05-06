#ifndef DATALOGGERPRIVATE_H
#define DATALOGGERPRIVATE_H

#include "vl_databaselogger.h"

class DataLoggerPrivate: public QObject
{
public:
    explicit DataLoggerPrivate(VeinLogger::DatabaseLogger *qPtr);
    ~DataLoggerPrivate();



    //component names

    VeinLogger::DatabaseLogger *m_qPtr = nullptr;
    friend class DatabaseLogger;
};

#endif // DATALOGGERPRIVATE_H
