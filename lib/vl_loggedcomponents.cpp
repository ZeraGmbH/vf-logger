#include "vl_loggedcomponents.h"

namespace VeinLogger
{

void LoggedComponents::clear()
{
    m_components.clear();
}

void LoggedComponents::addComponent(int entityId, const QString &componentName)
{
    m_components[entityId].insert(componentName);
}

bool LoggedComponents::contains(int entityId, const QString &componentName) const
{
    auto entityIter = m_components.constFind(entityId);
    if(entityIter != m_components.constEnd())
        return entityIter.value().contains(componentName);
    return false;
}

QList<int> LoggedComponents::getEntities() const
{
    return m_components.keys();
}

QStringList LoggedComponents::getComponents(int entityId) const
{
    QStringList components;
    auto iter = m_components.constFind(entityId);
    if(iter != m_components.constEnd())
        components = iter.value().values();
    return components;
}

}
