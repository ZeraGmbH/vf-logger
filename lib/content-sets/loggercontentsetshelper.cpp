#include "loggercontentsetshelper.h"

namespace VeinLogger
{

LoggerContentsetsHelper::LoggerContentsetsHelper(QList<int> entitiesWithAllComponentsStoredAlways,
                                                 const QStringList &contentSets) :
    m_loggedComponents(entitiesWithAllComponentsStoredAlways)
{
    EntityComponenMap entityComponenMap = LoggerContentSetConfig::entityComponentsFromContentSets(contentSets);
    for (auto iter = entityComponenMap.cbegin(); iter != entityComponenMap.cend(); ++iter) {
        const int entityId = iter.key();
        const QStringList componentList = iter.value();
        if (componentList.isEmpty())
            m_loggedComponents.addAllComponents(entityId);
        else
            for(auto &component : componentList)
                m_loggedComponents.addComponent(entityId, component);
    }
}

LoggerContentsetsHelper::LoggerContentsetsHelper(QList<int> entitiesWithAllComponentsStoredAlways,
                                                 QVariantMap entityComponentsVariantMap) :
    m_loggedComponents(entitiesWithAllComponentsStoredAlways)
{
    for (auto iter=entityComponentsVariantMap.cbegin(); iter!=entityComponentsVariantMap.cend(); ++iter) {
        const int entityId = iter.key().toInt();
        const QStringList componentList = iter.value().toStringList();
        if (componentList.isEmpty())
            m_loggedComponents.addAllComponents(entityId);
        else
            for(auto &component : componentList)
                m_loggedComponents.addComponent(entityId, component);
    }
}

EntityComponentData LoggerContentsetsHelper::getCurrentData(const VeinStorage::AbstractEventSystem *veinStorage) const
{
    EntityComponentData data;
    const VeinStorage::AbstractDatabase* storageDb = veinStorage->getDb();
    const QList<int> entities = m_loggedComponents.getEntities();
    for(const int entityId : entities) {
        QStringList componentNames;
        if(m_loggedComponents.areAllComponentsStored(entityId))
            componentNames = LoggedComponents::getComponentsFilteredForDb(storageDb, entityId);
        else
            componentNames = m_loggedComponents.getComponents(entityId);
        for(const QString &componentName : qAsConst(componentNames)) {
            if(storageDb->hasStoredValue(entityId, componentName)) {
                const QVariant componentData = storageDb->getStoredValue(entityId, componentName);
                data[entityId].append({componentName, componentData});
            }
        }
    }
    return data;
}

bool LoggerContentsetsHelper::isLoggedComponent(int entityId, const QString &componentName) const
{
    return m_loggedComponents.isLoggedComponent(entityId, componentName);
}

}
