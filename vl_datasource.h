#ifndef VEINLOGGER_DATASOURCE_H
#define VEINLOGGER_DATASOURCE_H

#include "vein-logger_global.h"
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
  class VEINLOGGERSHARED_EXPORT DataSource : public QObject
  {
    Q_OBJECT
  public:
    explicit DataSource(VeinApiQml::VeinQml *t_dataSource, QObject *t_parent=0);
    explicit DataSource(VeinStorage::VeinHash *t_dataSource, QObject *t_parent=0);
    ~DataSource() {}

    QVariant getValue(int t_entityId, const QString &t_componentName) const;
    QString getEntityName(int t_entityId) const;

  private:
    DataSourcePrivate *m_dPtr=0;
  };

} // namespace VeinLogger

#endif // VEINLOGGER_DATASOURCE_H
