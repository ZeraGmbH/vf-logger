#include "contentsetsotherfromcontentsetsconfig.h"
#include <zera-jsonfileloader.h>
#include <QDir>
#include <QJsonArray>

ContentSetsOtherFromContentSetsConfig::ContentSetsOtherFromContentSetsConfig()
{
}

void ContentSetsOtherFromContentSetsConfig::setConfigFileDir(const QString &dir)
{
    m_configFileDir = dir;
}

void ContentSetsOtherFromContentSetsConfig::setModmanSession(const QString &session)
{
    m_session = session;
    m_currentJsonContentSet = cJsonFileLoader::loadJsonFile(QDir::cleanPath(m_configFileDir + QDir::separator() + session));
}

QString ContentSetsOtherFromContentSetsConfig::getModmanSession()
{
    return m_session;
}

QStringList ContentSetsOtherFromContentSetsConfig::getAvailableContentSets()
{
    return m_currentJsonContentSet.keys();
}

QMap<int, QStringList> ContentSetsOtherFromContentSetsConfig::getEntityComponents(const QString &contentSetName)
{
    QMap<int, QStringList> ret;
    const auto ecArr = m_currentJsonContentSet[contentSetName].toArray();
    for(const auto &arrEntry : ecArr) {
        int entityId = arrEntry["EntityId"].toInt();
        QStringList componentList;
        const QJsonArray arrComponents = arrEntry["Components"].toArray();
        for(const auto &component : arrComponents) {
            componentList.append(component.toString());
        }
        ret.insert(entityId, componentList);
    }
    return ret;
}
