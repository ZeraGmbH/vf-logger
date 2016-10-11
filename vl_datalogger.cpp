#include "vl_datalogger.h"
#include "vl_postgresdatabase.h"
#include <QHash>

namespace VeinLogger
{
  class DataLoggerPrivate
  {
    explicit DataLoggerPrivate(DataLogger *t_qPtr) : m_qPtr(t_qPtr) {}

    // stores a list of record ids of type T and a key based on component names
    template <typename T>
    using ComponentRecordHash = QHash<QString, QList<T>>;

    //stores a hash of ComponentRecordHash with type T and a key based on entity ids
    template <typename T>
    using ECSRecordHash = QHash<int, ComponentRecordHash<T>>;

    //Tree formed structure to efficiently lookup dbrecord relations to entity/component types
    //storage topology of: entity hash -> component hash -> list of dbrecords
    //the value of ECS types may be logged to multiple dbrecords simultaneously
    ECSRecordHash<int> m_ecsRecordRelation;

    PostgresDatabase *m_database=0;

    DataLogger *m_qPtr=0;

    friend class DataLogger;
  };

  DataLogger::DataLogger(QObject *t_parent) : VeinEvent::EventSystem(t_parent)
  {
  }

  void DataLogger::setDatabase(PostgresDatabase *t_database)
  {
    m_dPtr->m_database=t_database;
  }

  bool DataLogger::processEvent(QEvent *t_event)
  {
    return false;
  }
}
