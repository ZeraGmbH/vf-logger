#ifndef VL_DATALOGGER_H
#define VL_DATALOGGER_H

#include "vein-logger_global.h"
#include <ve_eventsystem.h>

namespace VeinLogger
{
  class DataLoggerPrivate;
  class PostgresDatabase;

  class VEINLOGGERSHARED_EXPORT DataLogger : public VeinEvent::EventSystem
  {
    Q_OBJECT

  public:
    explicit DataLogger(QObject *t_parent=0);

    void setDatabase(PostgresDatabase *t_database);

    // EventSystem interface
  public:
    bool processEvent(QEvent *t_event) override;

  private:
    DataLoggerPrivate *m_dPtr=0;
  };

}

#endif // VL_DATALOGGER_H
