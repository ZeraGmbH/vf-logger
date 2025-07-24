#ifndef CONTENTSETSZERAALLFROMMODMANSESSIONS_H
#define CONTENTSETSZERAALLFROMMODMANSESSIONS_H

#include <vf_loggercontenthandler.h>
#include <QSet>
#include <QJsonObject>

class ContentSetsZeraAllFromModmanSessions : public LoggerContentHandler
{
public:
    ContentSetsZeraAllFromModmanSessions();
    void setConfigFileDir(const QString &dir) override;
    void setModmanSession(const QString &session) override;
    QString getModmanSession() override;
    QStringList getAvailableContentSets() override;
    QMap<int, QStringList> getEntityComponents(const QString &contentSetName) override;
private:
    QString m_session;
    QString m_configFileDir;
    QJsonObject m_currentJsonContentSet;

    const static QSet<int> m_entitiesNotAddedToZeraAllContentSet;
};

#endif // CONTENTSETSZERAALLFROMMODMANSESSIONS_H
