#include "jsonloggedvalues.h"
#include "loggercontentsetconfig.h"
#include <qjsonarray.h>

JsonLoggedValues::JsonLoggedValues(QStringList contentsetsList)
    : m_contentsetsList(contentsetsList)
{
}

void JsonLoggedValues::appendLoggedValues(QString entityId, QString componentName, QVariant value)
{
    if(!m_entityCompoValues.contains(entityId)) {
        QJsonObject compoValues;
        compoValues.insert(componentName, value.toJsonValue());
        m_entityCompoValues.insert(entityId, compoValues);
    }
    else {
        QJsonObject compoValues = m_entityCompoValues.value(entityId).toObject();
        compoValues.insert(componentName, value.toJsonValue());
        m_entityCompoValues[entityId] = compoValues;
    }
}

QJsonObject JsonLoggedValues::createLoggedValuesJson(QString sessionDeviceName)
{
    QJsonObject loggedValues;
    QJsonArray jsonArray;
    if(m_contentsetsList.contains("ZeraAll")) {
        QStringList contentsetAll = VeinLogger::LoggerContentSetConfig::getAvailableContentSets(sessionDeviceName);
        for (const QString &contentSet : contentsetAll)
            jsonArray.append(contentSet);
    }
    else
        for (const QString &contentSet : m_contentsetsList)
            jsonArray.append(contentSet);
    loggedValues.insert("contentsets", jsonArray);
    for (auto it = m_entityCompoValues.begin(); it != m_entityCompoValues.end(); ++it)
        loggedValues.insert(it.key(), it.value());

    return loggedValues;
}

