#ifndef VL_LOGGEDCOMPONENTS_H
#define VL_LOGGEDCOMPONENTS_H

#include <QString>
#include <QHash>
#include <QSet>

namespace VeinLogger
{

class LoggedComponents
{
public:
    void clear();
    void addComponent(int entityId, const QString &componentName);
    void addAllComponents(int entityId);
    bool isLoggedComponent(int entityId, const QString &componentName) const;
    bool areAllComponentsStored(int entityId);
    QList<int> getEntities() const;
    QStringList getComponents(int entityId) const;

private:
    bool specificContains(int entityId, const QString &componentName) const;

    QHash<int, QSet<QString>> m_entitiesWithSpecificComponents;
    QSet<int> m_entitiesWithAllComponents;
};

}
#endif // VL_LOGGEDCOMPONENTS_H
