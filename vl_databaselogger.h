#ifndef VL_DATALOGGER_H
#define VL_DATALOGGER_H

#include "vein-logger_global.h"
#include <ve_eventsystem.h>
#include <QDateTime>

namespace VeinLogger
{
  class DataLoggerPrivate;
  class SQLiteDB;
  class DataSource;

  class VEINLOGGERSHARED_EXPORT DatabaseLogger : public VeinEvent::EventSystem
  {
    Q_OBJECT

  public:
    explicit DatabaseLogger(SQLiteDB *t_database, DataSource *t_dataSource, QObject *t_parent=0);
    ~DatabaseLogger();

  signals:
    void sigAddLoggedValue(QVector<int> t_recordIds, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp);
    void sigAddEntity(int t_entityId, QString t_entityName);
    void sigAddComponent(const QString &t_componentName);

    void sigOpenDatabase(const QString &t_filePath);

  public slots:
    void startBatchTimer();

    // EventSystem interface
  public:
    bool processEvent(QEvent *t_event) override;

  private:
    DataLoggerPrivate *m_dPtr=0;
  };
}

#endif // VL_DATALOGGER_H
