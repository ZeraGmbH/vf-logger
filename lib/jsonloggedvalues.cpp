#include "jsonloggedvalues.h"
#include "loggercontentsetconfig.h"

JsonLoggedValues::JsonLoggedValues(QStringList contentsetsList, QObject *parent)
    : QObject{parent},
    m_contentsetsList(contentsetsList)
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

QJsonObject JsonLoggedValues::createLoggedValuesJson()
{
    QJsonObject loggedValues;

    if(m_contentsetsList.contains("ZeraAll")) {
        QStringList contentsetAll = VeinLogger::LoggerContentSetConfig::getAvailableContentSets();
        appendEntitiesOnContentset(loggedValues, contentsetAll);
    }
    else
        appendEntitiesOnContentset(loggedValues, m_contentsetsList);
    return loggedValues;
}

void JsonLoggedValues::appendEntitiesOnContentset(QJsonObject &loggedValues, QStringList contentsets)
{
    for(int i = 0; i < contentsets.size(); i++) {
        QMap<int, QStringList> componentsMap = VeinLogger::LoggerContentSetConfig::EntitiesComponentsLoggedFromContentSet(contentsets.at(i));
        for(int entity : componentsMap.keys()) {
            if(m_entityCompoValues.contains(QString::number(entity))) {
                loggedValues.insert(contentsets.at(i), m_entityCompoValues);
            }
        }
    }
}

