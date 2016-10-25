#include "vl_datalogger.h"
#include "vl_postgresdatabase.h"
#include <QHash>

#include <ve_commandevent.h>
#include <vcmp_componentdata.h>


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
    QVector<int> m_allowedIds = {1000, 1002, 1003, 1004, 1005, 1006, 1007, 1008, 1009, 1010, 1011, 1012, 1013, 1014, 1015};

    friend class DataLogger;
  };

  DataLogger::DataLogger(QObject *t_parent) : VeinEvent::EventSystem(t_parent), m_dPtr(new DataLoggerPrivate(this))
  {
  }

  void DataLogger::setDatabase(PostgresDatabase *t_database)
  {
    m_dPtr->m_database=t_database;
    m_dPtr->m_database->connectToDatabase();
  }

  bool DataLogger::processEvent(QEvent *t_event)
  {
    using namespace VeinEvent;
    using namespace VeinComponent;

    bool retVal = false;

    if(t_event->type()==CommandEvent::eventType())
    {
      CommandEvent *cEvent = 0;
      EventData *evData = 0;
      cEvent = static_cast<CommandEvent *>(t_event);
      Q_ASSERT(cEvent != 0);

      evData = cEvent->eventData();
      Q_ASSERT(evData != 0);

      if(cEvent->eventSubtype() == CommandEvent::EventSubtype::NOTIFICATION)
      {
        if (evData->type()==ComponentData::dataType() && evData->entityId() == 1005) //m_dPtr->m_allowedIds.contains(evData->entityId()))
        {
          ComponentData *cData=0;
          cData = static_cast<ComponentData *>(evData);
          m_dPtr->m_database->addEntity(cData->entityId());
          m_dPtr->m_database->addComponent(cData->componentName());
          m_dPtr->m_database->addLoggedValue(QVector<int>{1}, cData->entityId(), cData->componentName(), cData->newValue());
          retVal = true;
        }
      }
    }
    return retVal;
  }
}
