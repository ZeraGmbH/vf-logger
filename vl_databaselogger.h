#ifndef VL_DATALOGGER_H
#define VL_DATALOGGER_H

#include "vein-logger_global.h"
#include "vl_abstractloggerdb.h"
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
    explicit DatabaseLogger(DataSource *t_dataSource, VeinLogger::DBFactory t_factoryFunction, QObject *t_parent=0, AbstractLoggerDB::STORAGE_MODE t_storageMode=AbstractLoggerDB::STORAGE_MODE::TEXT);
    ~DatabaseLogger();
    void addScript(QmlLogger *t_script);
    void removeScript(QmlLogger *t_script);
    void addValueToLog(const QString &t_recordName, int t_entityId, const QString &t_componentName);
    bool loggingEnabled() const;

  signals:
    void sigAddLoggedValue(QVector<QString> t_recordIds, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp);
    void sigAddEntity(int t_entityId, const QString &t_entityName);
    void sigAddComponent(const QString &t_componentName);
    void sigAddRecord(const QString &t_recordName);

    void sigOpenDatabase(const QString &t_filePath);

    void sigDatabaseError(const QString &t_errorString);
    void sigDatabaseReady();
    void sigDatabaseUnloaded();
    void sigLoggingEnabledChanged(bool t_enabled);
    void sigLoggingStarted();
    void sigLoggingStopped();
    void sigLogSchedulerActivated();
    void sigLogSchedulerDeactivated();

  public slots:
    void setLoggingEnabled(bool t_enabled);
    bool openDatabase(const QString &t_filePath);
    void closeDatabase();

    // EventSystem interface
  public:
    bool processEvent(QEvent *t_event) override;

  private:
    void initEntity();

    DataLoggerPrivate *m_dPtr=nullptr;
  };
}

#endif // VL_DATALOGGER_H
