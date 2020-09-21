#ifndef VEINLOGGER_DATASOURCE_H
#define VEINLOGGER_DATASOURCE_H

#include "globalIncludes.h"
#include <QObject>

namespace VeinApiQml
{
class VeinQml;
}
namespace VeinStorage
{
class VeinHash;
}

namespace VeinLogger
{
class DataSourcePrivate;
class VFLOGGER_EXPORT DataSource : public QObject
{
    Q_OBJECT
public:
    explicit DataSource(VeinApiQml::VeinQml *t_dataSource, QObject *t_parent=nullptr);
    explicit DataSource(VeinStorage::VeinHash *t_dataSource, QObject *t_parent=nullptr);
    ~DataSource();

    QVariant getValue(int t_entityId, const QString &t_componentName) const;
    QString getEntityName(int t_entityId) const;

private:
    DataSourcePrivate *m_dPtr=nullptr;
};

} // namespace VeinLogger

#endif // VEINLOGGER_DATASOURCE_H
