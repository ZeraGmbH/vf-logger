#include "vl_loggedcomponents.h"

namespace VeinLogger
{

LoggedComponents::LoggedComponents(const QList<int> &entitiesWithAllComponentsStoredAlways) :
    m_entitiesWithAllComponentsStoredAlways(entitiesWithAllComponentsStoredAlways)
{
    initEntritiesStoredAlways();
}

void LoggedComponents::addComponent(int entityId, const QString &componentName)
{
    if(!m_entitiesWithAllComponents.contains(entityId))
        m_entitiesWithSpecificComponents[entityId].insert(componentName);
}

void LoggedComponents::addAllComponents(int entityId)
{
    m_entitiesWithSpecificComponents.remove(entityId);
    m_entitiesWithAllComponents.insert(entityId);
}

const QStringList LoggedComponents::getComponentsNotStored()
{
    return QStringList()
           << QStringLiteral("EntityName")
           << QStringLiteral("INF_ModuleInterface")
           << QStringLiteral("ACT_DEV_IFACE");
}

bool LoggedComponents::isLoggedComponent(int entityId, const QString &componentName) const
{
    if(m_entitiesWithAllComponents.contains(entityId))
        return !getComponentsNotStored().contains(componentName);
    return specificContains(entityId, componentName);
}

bool LoggedComponents::areAllComponentsStored(int entityId) const
{
    return m_entitiesWithAllComponents.contains(entityId);
}

QList<int> LoggedComponents::getEntities() const
{
    const QList<int> entitiesWithSpecificComponents = m_entitiesWithSpecificComponents.keys();
    const QList<int> entitiesWithAllComponents = m_entitiesWithAllComponents.values();
    return entitiesWithSpecificComponents + entitiesWithAllComponents;
}

QStringList LoggedComponents::getComponents(int entityId) const
{
    QStringList components;
    auto iter = m_entitiesWithSpecificComponents.constFind(entityId);
    if(iter != m_entitiesWithSpecificComponents.constEnd())
        components = iter.value().values();
    return components;
}

QStringList LoggedComponents::getComponentsFilteredForDb(const VeinStorage::AbstractDatabase *storageDb,
                                                         int entityId)
{
    QStringList fullList = storageDb->getComponentList(entityId);
    for(const auto &noStoreLabel : getComponentsNotStored())
        fullList.removeAll(noStoreLabel);
    return fullList;
}

bool LoggedComponents::specificContains(int entityId, const QString &componentName) const
{
    auto entityIter = m_entitiesWithSpecificComponents.constFind(entityId);
    if(entityIter != m_entitiesWithSpecificComponents.constEnd())
        return entityIter.value().contains(componentName);
    return false;
}

void VeinLogger::LoggedComponents::initEntritiesStoredAlways()
{
    for(auto entityIdStoredAlways : m_entitiesWithAllComponentsStoredAlways)
        m_entitiesWithAllComponents.insert(entityIdStoredAlways);
}

}
