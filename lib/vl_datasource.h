#ifndef VEINLOGGER_DATASOURCE_H
#define VEINLOGGER_DATASOURCE_H

#include "vflogger_export.h"
#include <ve_storagesystem.h>
#include <QObject>

namespace VeinApiQml
{
class VeinQml;
}

namespace VeinLogger
{
class DataSourcePrivate;
class VFLOGGER_EXPORT DataSource : public QObject
{
    Q_OBJECT
public:
    explicit DataSource(VeinEvent::StorageSystem *t_dataSource, QObject *t_parent=nullptr);
    ~DataSource();

    bool hasEntity(int t_entityId) const;
    QVariant getValue(int t_entityId, const QString &t_componentName) const;
    QString getEntityName(int t_entityId) const;
    QStringList getEntityComponentsForStore(int t_entityId);

    VeinEvent::StorageSystem *getStorageSystem();

private:
    DataSourcePrivate *m_dPtr = nullptr;
    VeinEvent::StorageSystem *m_storageSystem;
};

} // namespace VeinLogger

#endif // VEINLOGGER_DATASOURCE_H
