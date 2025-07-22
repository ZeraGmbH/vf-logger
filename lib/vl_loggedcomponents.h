#ifndef VL_LOGGEDCOMPONENTS_H
#define VL_LOGGEDCOMPONENTS_H

#include <QString>
#include <QHash>
#include <QSet>
#include <vs_abstracteventsystem.h>

namespace VeinLogger
{

class LoggedComponents
{
public:
    explicit LoggedComponents(QList<int> entitiesWithAllComponentsStoredAlways = QList<int>());
    void addComponent(int entityId, const QString &componentName);
    void addAllComponents(int entityId);
    void clear();

    bool isLoggedComponent(int entityId, const QString &componentName) const;
    bool areAllComponentsStored(int entityId);

    QList<int> getEntities() const;
    QStringList getComponents(int entityId) const;
    static QStringList getComponentsFilteredForDb(const VeinStorage::AbstractDatabase* storageDb,
                                                  int entityId);

private:
    void initEntritiesStoredAlways();
    bool specificContains(int entityId, const QString &componentName) const;
    static const QStringList getComponentsNotStored();

    QHash<int, QSet<QString>> m_entitiesWithSpecificComponents;
    QSet<int> m_entitiesWithAllComponents;
    const QList<int> m_entitiesWithAllComponentsStoredAlways;
};

}
#endif // VL_LOGGEDCOMPONENTS_H
