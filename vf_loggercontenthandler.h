#ifndef LOGGERCONTENTHANDLER_H
#define LOGGERCONTENTHANDLER_H

#include <QStringList>
#include <QMap>

class LoggerContentHandler
{
public:
    virtual void setConfigFileDir(const QString &dir) = 0;
    virtual QStringList getAvailableContentSets(const QString &session) = 0;
    virtual QMap<int, QStringList> getEntityComponents(const QString &contentSetName) = 0;
};

#endif // LOGGERCONTENTHANDLER_H
