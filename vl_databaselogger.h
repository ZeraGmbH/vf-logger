#ifndef VL_DATALOGGER_H
#define VL_DATALOGGER_H

#include "vein-logger_global.h"
#include <ve_eventsystem.h>
#include <QDateTime>

namespace VeinLogger
{
  class DataLoggerPrivate;
  class DataSource;
  class QmlLogger;

  class VEINLOGGERSHARED_EXPORT DatabaseLogger : public VeinEvent::EventSystem
  {
    Q_OBJECT

  public:
    explicit DatabaseLogger(DataSource *t_dataSource, QObject *t_parent=0);
    ~DatabaseLogger();
    void addScript(QmlLogger *t_script);
    void removeScript(QmlLogger *t_script);
    void addValueToLog(const QString &t_recordName, int t_entityId, const QString &t_componentName);
    bool loggingEnabled() const;
    static int entityId();

  signals:
    void sigAddLoggedValue(QVector<QString> t_recordIds, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp);
    void sigAddEntity(int t_entityId, const QString &t_entityName);
    void sigAddComponent(const QString &t_componentName);
    void sigAddRecord(const QString &t_recordName);

    void sigOpenDatabase(const QString &t_filePath);
    void sigDatabaseError(const QString &t_filePath);
    void sigLoggingEnabledChanged(bool t_enabled);

  public slots:
    void setLoggingEnabled(bool t_enabled);
    bool openDatabase(const QString &t_filePath);

    // EventSystem interface
  public:
    bool processEvent(QEvent *t_event) override;

  private:
    void initEntity();

    DataLoggerPrivate *m_dPtr=0;
  };
}

#endif // VL_DATALOGGER_H
