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
        QMap<int, QStringList> componentsMap = VeinLogger::LoggerContentSetConfig::componentFromContentSet(contentsets.at(i));
        for(int entity : componentsMap.keys()) {
            QJsonObject obj;
            if(m_entityCompoValues.contains(QString::number(entity))) {
                obj = m_entityCompoValues.value(QString::number(entity)).toObject();
                appendToJson(loggedValues, contentsets.at(i), obj);
            }
        }
    }
}

void JsonLoggedValues::appendToJson(QJsonObject &json, QString contentSet, QJsonObject valuesToAppend)
{
    if(!json.contains(contentSet))
        json.insert(contentSet, valuesToAppend);
    else {
        QJsonObject tempObj = json.value(contentSet).toObject();
        for(auto it = valuesToAppend.constBegin(); it != valuesToAppend.constEnd(); it++)
            tempObj.insert(it.key(), it.value());
        json[contentSet] = tempObj;
    }
}

