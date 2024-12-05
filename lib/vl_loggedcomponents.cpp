#include "vl_loggedcomponents.h"
#include "vl_globallabels.h"

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

bool LoggedComponents::isLoggedComponent(int entityId, const QString &componentName) const
{
    bool storeComponent = false;
    if(contains(entityId, VLGlobalLabels::allComponentsName())) {
        QStringList componentsNoStore = VLGlobalLabels::noStoreComponents();
        storeComponent = !componentsNoStore.contains(componentName);
    }
    else
        storeComponent = contains(entityId, componentName);
    return storeComponent;
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
