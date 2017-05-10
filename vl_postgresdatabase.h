#ifndef VEINLOGGER_POSTGRESDATABASE_H
#define VEINLOGGER_POSTGRESDATABASE_H

#include "vein-logger_global.h"

#include <QObject>
#include <QDateTime>
#include <QVector>
#include <QtSql>

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

  class PostgresDBPrivate;

  class VEINLOGGERSHARED_EXPORT PostgresDatabase : public QObject
  {
    Q_OBJECT
  public:
    explicit PostgresDatabase(QObject *t_parent = 0);

  signals:

  public slots:
    /**
     * @b Stores a value in the database
     * @param t_timestamp defaults to the time when the function will be called
     */
    void addLoggedValue(QVector<int> t_recordIds, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp = QDateTime::currentDateTime());
    void addEntity(int t_entityId);
    void addComponent(const QString &t_componentName);
    void addRecord(const QString &t_recordName);
    void runBatchedExecution();
    bool connectToDatabase();

  private:
    void initLocalData();
    QString convertDoubleArrayToString(QVariant t_value);

    PostgresDBPrivate *m_dPtr;
  };

} // namespace VeinLogger



#endif // VEINLOGGER_POSTGRESDATABASE_H
