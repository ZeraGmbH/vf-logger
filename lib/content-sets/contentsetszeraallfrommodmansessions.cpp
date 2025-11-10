#include "contentsetszeraallfrommodmansessions.h"
#include <zera-jsonfileloader.h>
#include <QDir>
#include <QJsonArray>


const QSet<int> ContentSetsZeraAllFromModmanSessions::m_entitiesNotAddedToZeraAllContentSet =
    // apimodule / hotplugcontrolsmodule / scpimodule
    QSet<int>() << 1500 << 1700 << 9999;

ContentSetsZeraAllFromModmanSessions::ContentSetsZeraAllFromModmanSessions()
{
}

void ContentSetsZeraAllFromModmanSessions::setConfigFileDir(const QString &dir)
{
    m_configFileDir = dir;
}

void ContentSetsZeraAllFromModmanSessions::setModmanSession(const QString &session)
{
    m_session = session;
    m_currentJsonContentSet = cJsonFileLoader::loadJsonFile(QDir::cleanPath(m_configFileDir + QDir::separator() + session));
}

QString ContentSetsZeraAllFromModmanSessions::getModmanSession()
{
    return m_session;
}

QStringList ContentSetsZeraAllFromModmanSessions::getAvailableContentSets()
{
    QStringList ret;
    if(!m_currentJsonContentSet["modules"].toArray().isEmpty()) {
        ret.append("ZeraAll");
    }
    return ret;
}

QMap<int, QStringList> ContentSetsZeraAllFromModmanSessions::getEntityComponents(const QString &contentSetName)
{
    QMap<int, QStringList> ret;
    if(!getAvailableContentSets().isEmpty() && contentSetName == "ZeraAll") {
        const auto ecArr = m_currentJsonContentSet["modules"].toArray();
        for(const auto &arrEntry : ecArr) {
            int entityId = arrEntry["id"].toInt();
            if (!m_entitiesNotAddedToZeraAllContentSet.contains(entityId))
                ret.insert(entityId, QStringList());
        }
    }
    return ret;
}
