#include "vl_loggedcomponents.h"

namespace VeinLogger
{

void LoggedComponents::clear()
{
    m_entitiesWithSpecificComponents.clear();
    m_entitiesWithAllComponents.clear();
}

void LoggedComponents::addComponent(int entityId, const QString &componentName)
{
    if(!m_entitiesWithAllComponents.contains(entityId)) // TODO: we have no tests for this
        m_entitiesWithSpecificComponents[entityId].insert(componentName);
}

void LoggedComponents::addAllComponents(int entityId)
{
    m_entitiesWithSpecificComponents.remove(entityId);
    m_entitiesWithAllComponents.insert(entityId);
}

bool LoggedComponents::specificContains(int entityId, const QString &componentName) const
{
    auto entityIter = m_entitiesWithSpecificComponents.constFind(entityId);
    if(entityIter != m_entitiesWithSpecificComponents.constEnd())
        return entityIter.value().contains(componentName);
    return false;
}

const QStringList LoggedComponents::getComponentsNotStoredOnAll()
{
    return QStringList()
           << QStringLiteral("EntityName")
           << QStringLiteral("INF_ModuleInterface");
}

bool LoggedComponents::isLoggedComponent(int entityId, const QString &componentName) const
{
    if(m_entitiesWithAllComponents.contains(entityId))
        return !getComponentsNotStoredOnAll().contains(componentName);
    return specificContains(entityId, componentName);
}

bool LoggedComponents::areAllComponentsStored(int entityId)
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

QStringList LoggedComponents::removeNotStoredComponents(const QStringList &allComponents)
{
    QStringList ret = allComponents;
    for(const auto &noStoreLabel : getComponentsNotStoredOnAll())
        ret.removeAll(noStoreLabel);
    return ret;
}

}
