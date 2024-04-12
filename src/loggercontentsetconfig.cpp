#include "loggercontentsetconfig.h"

namespace VeinLogger
{
QList<LoggerContentSetConfig::LoggerContentConfigEntry> LoggerContentSetConfig::m_loggerConfigEnvironment;

void LoggerContentSetConfig::setJsonEnvironment(const QString configFileDir, std::shared_ptr<LoggerContentHandler> loggerContentHandler)
{
    LoggerContentConfigEntry newConfig = { configFileDir, loggerContentHandler };
    newConfig.m_loggerContentHandler->setConfigFileDir(newConfig.m_configFileDir);
    m_loggerConfigEnvironment.append(newConfig);
}

const QList<LoggerContentSetConfig::LoggerContentConfigEntry> &LoggerContentSetConfig::getConfigEnvironment()
{
    return m_loggerConfigEnvironment;
}

QStringList LoggerContentSetConfig::getAvailableContentSets()
{
    QStringList ret;
    for(auto &confEnv: m_loggerConfigEnvironment)
        ret.append(confEnv.m_loggerContentHandler->getAvailableContentSets());
    return ret;
}
}
