#ifndef VEINLOGGER_POSTGRESDATABASE_H
#define VEINLOGGER_POSTGRESDATABASE_H

#include "vein-logger_global.h"

#include <QObject>
#include <QDateTime>
#include <QVector>
#include <QtSql>


namespace VeinLogger
{

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
    void addLoggedValue(int t_recordId, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp = QDateTime::currentDateTime());

    void addComponent(const QString &t_componentName);
    void addEntity(int t_entityId, const QString &t_entityName);
    void addRecord(const QString &t_recordName);
  signals:

  public slots:

  private:
    QHash<QString, int> m_recordIds;
    QHash<QString, int> m_entityIds;
    QHash<QString, int> m_componentIds;

    QSqlQuery m_valueMappingQuery;
    QSqlQuery m_recordMappingQuery;
    QSqlQuery m_valueMappingSequenceQuery;
    QSqlQuery m_valuesDoubleQuery;
    QSqlQuery m_valuesIntQuery;
    QSqlQuery m_valuesStringQuery;
    QSqlQuery m_valuesDoubleListQuery;
    QSqlQuery m_componentQuery;
    QSqlQuery m_componentSequenceQuery;
    QSqlQuery m_entityQuery;
    QSqlQuery m_recordQuery;
    QSqlQuery m_recordSequenceQuery;

    QSqlDatabase m_logDB;
  };

} // namespace VeinLogger



#endif // VEINLOGGER_POSTGRESDATABASE_H
