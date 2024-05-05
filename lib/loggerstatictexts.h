#ifndef LOGGERSTATICTEXTS_H
#define LOGGERSTATICTEXTS_H

#include <QLatin1String>

class LoggerStaticTexts
{
public:
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
};

#endif // LOGGERSTATICTEXTS_H
