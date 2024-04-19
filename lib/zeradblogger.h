#ifndef ZERADBLOGGER_H
#define ZERADBLOGGER_H

#include <vl_databaselogger.h>

namespace VeinLogger
{
class DataSource;
}

/**
 * @brief Class to avoid adding policy to the VeinLogger::DatabaseLogger
 */
class ZeraDBLogger : public VeinLogger::DatabaseLogger
{
    Q_OBJECT
public:
    ZeraDBLogger(VeinLogger::DataSource *t_dataSource, VeinLogger::DBFactory t_factoryFunction, QObject *parent=nullptr);
};

#endif // ZERADBLOGGER_H
