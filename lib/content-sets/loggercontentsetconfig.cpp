#include "loggercontentsetconfig.h"
#include "vl_componentunion.h"

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

QVariantMap LoggerContentSetConfig::componentFromContentSets(const QStringList &contentSets)
{
    typedef QMap<int, QStringList> EntityComponenMap;
    EntityComponenMap tmpResultMap;
    for(auto &contentSet : contentSets) {
        for(auto &confEnv: LoggerContentSetConfig::getConfigEnvironment()) {
            EntityComponenMap ecMap = confEnv.m_loggerContentHandler->getEntityComponents(contentSet);
            for(EntityComponenMap::const_iterator iter=ecMap.constBegin(); iter!=ecMap.constEnd(); ++iter)
                ComponentUnion::uniteComponents(tmpResultMap, iter.key(), iter.value());
        }
    }
    QVariantMap resultMap;
    EntityComponenMap::const_iterator iter;
    for(iter=tmpResultMap.constBegin(); iter!=tmpResultMap.constEnd(); ++iter)
        resultMap[QString::number(iter.key())] = tmpResultMap[iter.key()];
    return resultMap;
}

}
