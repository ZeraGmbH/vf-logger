#ifndef VEINLOGGER_SQLITEDB_H
#define VEINLOGGER_SQLITEDB_H

#include "vein-logger_global.h"

#include <QObject>
#include <QVector>
#include <QDateTime>
#include <QVariant>


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

  class VEINLOGGERSHARED_EXPORT SQLiteDB : public QObject
  {
    Q_OBJECT
  public:
    explicit SQLiteDB(QObject *t_parent = 0);
    ~SQLiteDB();

  public slots:
    void initLocalData();
    void addComponent(const QString &t_componentName);
    void addEntity(int t_entityId, QString t_entityName);
    void addRecord(const QString &t_recordName);
    void addLoggedValue(QVector<int> t_recordIds, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp);

    bool openDatabase(const QString &t_dbPath);
    void runBatchedExecution();

  private:
    QString convertDoubleArrayToString(QVariant t_value);

    DBPrivate *m_dPtr=0;
  };

} // namespace VeinLogger

#endif // VEINLOGGER_SQLITEDB_H
