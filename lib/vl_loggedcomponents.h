#ifndef VL_LOGGEDCOMPONENTS_H
#define VL_LOGGEDCOMPONENTS_H

#include <QString>
#include <QMap>
#include <QSet>

namespace VeinLogger
{

class LoggedComponents
{
public:
    LoggedComponents(QList<int> entitiesWithAllComponentsStoredAlways = QList<int>());
    void addComponent(int entityId, const QString &componentName);
    void addAllComponents(int entityId);
    void clear();

    bool isLoggedComponent(int entityId, const QString &componentName) const;
    bool areAllComponentsStored(int entityId);

    QList<int> getEntities() const;
    QStringList getComponents(int entityId) const;
    static QStringList removeNotStoredOnEntitiesStoringAllComponents(const QStringList &allComponents);

private:
    void initEntritiesStoredAlways();
    bool specificContains(int entityId, const QString &componentName) const;
    static const QStringList getComponentsNotStoredOnAll();

    QHash<int, QSet<QString>> m_entitiesWithSpecificComponents;
    QSet<int> m_entitiesWithAllComponents;
    const QList<int> m_entitiesWithAllComponentsStoredAlways;
};

}
#endif // VL_LOGGEDCOMPONENTS_H
