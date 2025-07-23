#ifndef LOGGERCONTENTSETSHELPER_H
#define LOGGERCONTENTSETSHELPER_H

#include "vl_loggedcomponents.h"
#include <vs_abstractdatabase.h>

namespace VeinLogger
{
struct Component {
    QString name;
    QVariant data;
};

typedef QMap<int, QList<Component>> EntityComponentData;

class LoggerContentsetsHelper
{
public:
    explicit LoggerContentsetsHelper(const QList<int> &entitiesWithAllComponentsStoredAlways,
                                     const QStringList &contentSets);
    explicit LoggerContentsetsHelper(const QList<int> &entitiesWithAllComponentsStoredAlways, // for compatibility -> remove later
                                     const QVariantMap &entityComponentsVariantMap);
    EntityComponentData getCurrentData(const VeinStorage::AbstractEventSystem *veinStorage) const;
    bool isLoggedComponent(int entityId, const QString &componentName) const;
private:
    LoggedComponents m_loggedComponents;
};

}
#endif // LOGGERCONTENTSETSHELPER_H
