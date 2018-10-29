#ifndef VEINLOGGER_ABSTRACTLOGGERDB_H
#define VEINLOGGER_ABSTRACTLOGGERDB_H

#include <QObject>
#include <QVector>
#include <QDateTime>
#include <QVariant>
#include <functional>

namespace VeinLogger
{
  class AbstractLoggerDB : public QObject
  {
    Q_OBJECT

  public:
    AbstractLoggerDB(QObject *t_parent=nullptr);
    virtual ~AbstractLoggerDB()
    {

    }

    enum class STORAGE_MODE : int {
      TEXT = 0,
      BINARY = 1,
    };

    virtual bool hasEntityId(int t_entityId) const =0;
    virtual bool hasComponentName(const QString &t_componentName) const =0;
    virtual bool hasRecordName(const QString &t_recordName) const =0;

    virtual bool databaseIsOpen() const =0;
    virtual QString databasePath() const =0;
    virtual void setStorageMode(STORAGE_MODE t_storageMode) =0;
    virtual STORAGE_MODE getStorageMode() const =0;
    virtual std::function<bool(QString)> getDatabaseValidationFunction() const =0;

  signals:
    void sigDatabaseError(const QString &t_errorString);
    void sigDatabaseReady();

  public slots:
    virtual void initLocalData() =0;
    virtual void addComponent(const QString &t_componentName) =0;
    virtual void addEntity(int t_entityId, QString t_entityName) =0;
    virtual int addRecord(const QString &t_recordName) =0;
    virtual void addLoggedValue(QVector<int> t_recordIds, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp) =0;
    virtual void addLoggedValue(QVector<QString> t_recordNames, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp) =0;

    virtual bool openDatabase(const QString &t_dbPath) =0;
    virtual void runBatchedExecution() =0;
  };

  /// @b factory function alias to create database
  using DBFactory = std::function<AbstractLoggerDB *()>;
} // namespace VeinLogger

#endif // VEINLOGGER_ABSTRACTLOGGERDB_H
