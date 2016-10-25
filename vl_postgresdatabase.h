#ifndef VEINLOGGER_POSTGRESDATABASE_H
#define VEINLOGGER_POSTGRESDATABASE_H

#include "vein-logger_global.h"

#include <QObject>
#include <QDateTime>
#include <QVector>
#include <QtSql>
#include <QTimer>
#include <QTextStream>

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

  class VEINLOGGERSHARED_EXPORT PostgresDatabase : public QObject
  {
    Q_OBJECT
  public:
    explicit PostgresDatabase(QObject *t_parent = 0);
    bool connectToDatabase();

    /**
     * @b Stores a value in the database
     * @param t_timestamp defaults to the time when the function will be called
     */
    void addLoggedValue(QVector<int> t_recordIds, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp = QDateTime::currentDateTime());

    void addComponent(const QString &t_componentName);
    void addEntity(int t_entityId);
    void addRecord(const QString &t_recordName);
  signals:

  public slots:
    void runBatchedExecution();

  private:
    void initLocalData();

    QString convertDoubleArrayToString(QVariant t_value);


    QHash<QString, int> m_recordIds;
    QVector<int> m_entityIds;
    QHash<QString, int> m_componentIds;

    QVector<SQLBatchData> m_batchVector;

    QTimer m_batchTimer;

    QTextStream m_fooStream;

    //commonly used queries
    QSqlQuery m_valueMappingQuery;
    QSqlQuery m_valueMappingSequenceQuery;
    QSqlQuery m_valueMappingSequenceSetterQuery;
    QSqlQuery m_recordMappingQuery;
    QSqlQuery m_valuesDoubleQuery;
    QSqlQuery m_valuesIntQuery;
    QSqlQuery m_valuesStringQuery;
    QSqlQuery m_valuesDoubleListQuery;
    QSqlQuery m_componentQuery;
    QSqlQuery m_componentSequenceQuery;
    QSqlQuery m_entityQuery;
    QSqlQuery m_recordQuery;
    QSqlQuery m_recordSequenceQuery;

    int m_valueMappingQueryCounter=0;

    QSqlDatabase m_logDB;
  };

} // namespace VeinLogger



#endif // VEINLOGGER_POSTGRESDATABASE_H
