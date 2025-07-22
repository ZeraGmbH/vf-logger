#ifndef LOGGERCONTENTSETSHELPER_H
#define LOGGERCONTENTSETSHELPER_H

#include "loggercontentsetconfig.h"
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
    explicit LoggerContentsetsHelper(QList<int> entitiesWithAllComponentsStoredAlways,
                                     const QStringList &contentSets);
    explicit LoggerContentsetsHelper(QList<int> entitiesWithAllComponentsStoredAlways, // for compatibility -> remove later
                                     QVariantMap entityComponentsVariantMap);
    EntityComponentData getCurrentData(const VeinStorage::AbstractEventSystem *veinStorage) const;
    bool isLoggedComponent(int entityId, const QString &componentName) const;
private:
    LoggedComponents m_loggedComponents;
};

}
#endif // LOGGERCONTENTSETSHELPER_H
