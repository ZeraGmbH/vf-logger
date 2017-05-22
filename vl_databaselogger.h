#ifndef VL_DATALOGGER_H
#define VL_DATALOGGER_H

#include "vein-logger_global.h"
#include <ve_eventsystem.h>
#include <QDateTime>

namespace VeinLogger
{
  class DataLoggerPrivate;
  class SQLiteDB;

  class VEINLOGGERSHARED_EXPORT DatabaseLogger : public VeinEvent::EventSystem
  {
    Q_OBJECT

  public:
    explicit DatabaseLogger(QObject *t_parent=0);
    ~DatabaseLogger();

    void setDatabase(SQLiteDB *t_database);

  signals:
    void sigAddLoggedValue(QVector<int> t_recordIds, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp);
    void sigAddEntity(int t_entityId);
    void sigAddComponent(const QString &t_componentName);

    void sigOpenDatabase(const QString &t_filePath);
    void sigDBReady();

    // EventSystem interface
  public:
    bool processEvent(QEvent *t_event) override;

  private slots:
    void startBatchTimer();

  private:
    DataLoggerPrivate *m_dPtr=0;
  };
}

#endif // VL_DATALOGGER_H
