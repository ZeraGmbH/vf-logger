#ifndef VL_DATALOGGER_H
#define VL_DATALOGGER_H

#include "vein-logger_global.h"
#include <ve_eventsystem.h>
#include <QDateTime>

namespace VeinLogger
{
  class DataLoggerPrivate;
  class PostgresDatabase;

  class VEINLOGGERSHARED_EXPORT DataLogger : public VeinEvent::EventSystem
  {
    Q_OBJECT

  public:
    explicit DataLogger(QObject *t_parent=0);
    ~DataLogger();

    void setDatabase(PostgresDatabase *t_database);

  signals:
    void sigAddLoggedValue(QVector<int> t_recordIds, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp);
    void sigAddEntity(int t_entityId);
    void sigAddComponent(QString t_componentName);

    // EventSystem interface
  public:
    bool processEvent(QEvent *t_event) override;

  private:
    DataLoggerPrivate *m_dPtr=0;
  };

}

#endif // VL_DATALOGGER_H
