#ifndef TESTLOGGERDB_H
#define TESTLOGGERDB_H

#include "vl_abstractloggerdb.h"

class TestLoggerDB : public VeinLogger::AbstractLoggerDB
{
    Q_OBJECT
public:
    TestLoggerDB();
};

#endif // TESTLOGGERDB_H
