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
    static const QLatin1String s_entityNameComponentName;
    static const QLatin1String s_loggingStatusTextComponentName;
    static const QLatin1String s_loggingEnabledComponentName;
    static const QLatin1String s_databaseReadyComponentName;
    static const QLatin1String s_databaseFileComponentName;
    static const QLatin1String s_scheduledLoggingEnabledComponentName;
    static const QLatin1String s_scheduledLoggingDurationComponentName;
    static const QLatin1String s_scheduledLoggingCountdownComponentName;
    static const QLatin1String s_existingSessionsComponentName;

    static const QLatin1String s_customerDataComponentName;
    static const QLatin1String s_sessionNameComponentName;
    static const QLatin1String s_guiContextComponentName;
    static const QLatin1String s_transactionNameComponentName;
    static const QLatin1String s_currentContentSetsComponentName;
    static const QLatin1String s_availableContentSetsComponentName;
    static const QLatin1String loggedComponentsComponentName;

    VeinLogger::DatabaseLogger *m_qPtr = nullptr;
    friend class DatabaseLogger;
};

#endif // DATALOGGERPRIVATE_H
