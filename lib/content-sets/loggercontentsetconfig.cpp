#include "loggercontentsetconfig.h"
#include "vl_componentunion.h"
#include <qdebug.h>

namespace VeinLogger
{

QList<LoggerContentSetConfig::LoggerContentConfigEntry> LoggerContentSetConfig::m_loggerConfigEnvironment;

void LoggerContentSetConfig::setJsonEnvironment(const QString &configFileDir,
                                                std::shared_ptr<LoggerContentHandler> loggerContentHandler)
{
    for (const LoggerContentConfigEntry &config : qAsConst(m_loggerConfigEnvironment)) {
        if(config.m_configFileDir == configFileDir) {
            qInfo() << "VeinLogger - the config path" << configFileDir << "is already set.";
            return;
        }
    }
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

QStringList LoggerContentSetConfig::getAvailableContentSets(QString sessionDeviceName)
{
    QStringList ret;
    if(sessionDeviceName.isEmpty())
        return getAvailableContentSets();
    for(auto &confEnv: m_loggerConfigEnvironment) {
        QString activeSession = confEnv.m_loggerContentHandler->getModmanSession();
        confEnv.m_loggerContentHandler->setModmanSession(sessionDeviceName);
        ret.append(confEnv.m_loggerContentHandler->getAvailableContentSets());
        confEnv.m_loggerContentHandler->setModmanSession(activeSession);
    }
    return ret;
}

QVariantMap LoggerContentSetConfig::componentFromContentSets(const QStringList &contentSets)
{
    EntityComponenMap entityComponentMap = entityComponentsFromContentSets(contentSets);
    QVariantMap resultMap;
    for (auto iter=entityComponentMap.constBegin(); iter!=entityComponentMap.constEnd(); ++iter)
        resultMap[QString::number(iter.key())] = entityComponentMap[iter.key()];
    return resultMap;
}

EntityComponenMap LoggerContentSetConfig::entityComponentsFromContentSets(const QStringList &contentSets)
{
    EntityComponenMap resultMap;
    for(auto &contentSet : contentSets) {
        for(auto &confEnv: LoggerContentSetConfig::getConfigEnvironment()) {
            EntityComponenMap ecMap = confEnv.m_loggerContentHandler->getEntityComponents(contentSet);
            for(EntityComponenMap::const_iterator iter=ecMap.constBegin(); iter!=ecMap.constEnd(); ++iter)
                ComponentUnion::uniteComponents(resultMap, iter.key(), iter.value());
        }
    }
    return resultMap;
}

}
