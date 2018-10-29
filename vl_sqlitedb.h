#ifndef VEINLOGGER_SQLITEDB_H
#define VEINLOGGER_SQLITEDB_H

#include "vein-logger_global.h"
#include "vl_abstractloggerdb.h"

#include <QVector>
#include <QDateTime>
#include <QVariant>

#include <functional>


namespace VeinLogger
{
  struct SQLBatchData
  {
    int entityId;
    int componentId;
    QVector<int> recordIds;
    QDateTime timestamp;
    QVariant value;
  };

  class DBPrivate;

  class VEINLOGGERSHARED_EXPORT SQLiteDB : public AbstractLoggerDB
  {
    Q_OBJECT
  public:

    explicit SQLiteDB(QObject *t_parent = nullptr);
    ~SQLiteDB();

    bool hasEntityId(int t_entityId) const override;
    bool hasComponentName(const QString &t_componentName) const override;
    bool hasRecordName(const QString &t_recordName) const override;

    bool databaseIsOpen() const override;
    QString databasePath() const override;
    void setStorageMode(AbstractLoggerDB::STORAGE_MODE t_storageMode) override;
    AbstractLoggerDB::STORAGE_MODE getStorageMode() const override;
    std::function<bool(QString)> getDatabaseValidationFunction() const override;

    static bool isValidDatabase(QString t_dbPath);

  public slots:
    void initLocalData() override;
    void addComponent(const QString &t_componentName) override;
    void addEntity(int t_entityId, QString t_entityName) override;
    int addRecord(const QString &t_recordName) override;
    void addLoggedValue(QVector<int> t_recordIds, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp) override;
    void addLoggedValue(QVector<QString> t_recordNames, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp) override;

    bool openDatabase(const QString &t_dbPath) override;


    void runBatchedExecution() override;

  private:
    DBPrivate *m_dPtr=nullptr;
  };

} // namespace VeinLogger

#endif // VEINLOGGER_SQLITEDB_H
