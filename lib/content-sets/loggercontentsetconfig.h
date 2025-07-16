#ifndef LOGGERCONTENTSETCONFIG_H
#define LOGGERCONTENTSETCONFIG_H

#include <vf_loggercontenthandler.h>
#include <QVariantMap>
#include <memory>

namespace VeinLogger
{
class LoggerContentSetConfig
{
public:
    static void setJsonEnvironment(const QString &configFileDir,
                                   std::shared_ptr<LoggerContentHandler> loggerContentHandler);
    struct LoggerContentConfigEntry {
        QString m_configFileDir;
        std::shared_ptr<LoggerContentHandler> m_loggerContentHandler;
    };
    static const QList<LoggerContentConfigEntry> &getConfigEnvironment();
    static QStringList getAvailableContentSets();
    static QVariantMap componentFromContentSets(const QStringList &contentSets);
    static QMap<int, QStringList> EntitiesComponentsLoggedFromContentSet(const QString &contentSet, QString sessionDeviceName);

private:
    static QList<LoggerContentConfigEntry> m_loggerConfigEnvironment;
};
}

#endif // LOGGERCONTENTSETCONFIG_H
