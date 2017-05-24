#include "vl_databaselogger.h"
#include "vl_sqlitedb.h"
#include "vl_datasource.h"
#include <QHash>
#include <QThread>
#include <QMetaObject>
#include <QCoreApplication>
#include <QTimer>

#include <ve_commandevent.h>
#include <vcmp_componentdata.h>


namespace VeinLogger
{
  class DataLoggerPrivate
  {
    explicit DataLoggerPrivate(DatabaseLogger *t_qPtr) : m_qPtr(t_qPtr)
    {
      m_batchedExecutionTimer.setInterval(1000);
      m_batchedExecutionTimer.setSingleShot(false);
    }

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

    SQLiteDB *m_database=0;
    DataSource *m_dataSource=0;

    DatabaseLogger *m_qPtr=0;
    QVector<int> m_disallowedIds = {0, 50}; ///< @todo replace with user defined whitelist
    QThread m_asyncDatabaseThread;
    QTimer m_batchedExecutionTimer;
    friend class DatabaseLogger;
  };

  DatabaseLogger::DatabaseLogger(SQLiteDB *t_database, DataSource *t_dataSource, QObject *t_parent) : VeinEvent::EventSystem(t_parent), m_dPtr(new DataLoggerPrivate(this))
  {
    m_dPtr->m_dataSource=t_dataSource;
    m_dPtr->m_asyncDatabaseThread.setObjectName("VFLogDBThread");

    m_dPtr->m_database=t_database;
    m_dPtr->m_database->moveToThread(&m_dPtr->m_asyncDatabaseThread);
    m_dPtr->m_asyncDatabaseThread.start();

    //will be queued connection due to thread affinity
    connect(this, SIGNAL(sigAddLoggedValue(QVector<int>,int,QString,QVariant,QDateTime)), m_dPtr->m_database, SLOT(addLoggedValue(QVector<int>,int,QString,QVariant,QDateTime)));
    connect(this, SIGNAL(sigAddEntity(int, QString)), m_dPtr->m_database, SLOT(addEntity(int, QString)));
    connect(this, SIGNAL(sigAddComponent(QString)), m_dPtr->m_database, SLOT(addComponent(QString)));

    connect(this, SIGNAL(sigOpenDatabase(QString)), m_dPtr->m_database, SLOT(openDatabase(QString)));
    connect(&m_dPtr->m_batchedExecutionTimer, SIGNAL(timeout()), m_dPtr->m_database, SLOT(runBatchedExecution()));
  }

  DatabaseLogger::~DatabaseLogger()
  {
    m_dPtr->m_batchedExecutionTimer.stop();
    m_dPtr->m_asyncDatabaseThread.quit();
    m_dPtr->m_asyncDatabaseThread.wait();
    delete m_dPtr;
  }

  bool DatabaseLogger::processEvent(QEvent *t_event)
  {
    using namespace VeinEvent;
    using namespace VeinComponent;

    bool retVal = false;

    if(m_dPtr->m_batchedExecutionTimer.isActive() && t_event->type()==CommandEvent::eventType())
    {
      CommandEvent *cEvent = 0;
      EventData *evData = 0;
      cEvent = static_cast<CommandEvent *>(t_event);
      Q_ASSERT(cEvent != 0);

      evData = cEvent->eventData();
      Q_ASSERT(evData != 0);

      if(cEvent->eventSubtype() == CommandEvent::EventSubtype::NOTIFICATION)
      {
        if(evData->type()==ComponentData::dataType() && m_dPtr->m_disallowedIds.contains(evData->entityId()) == false)
        {
          ComponentData *cData=0;
          cData = static_cast<ComponentData *>(evData);
          emit sigAddEntity(cData->entityId(), m_dPtr->m_dataSource->getEntityName(cData->entityId()));
          emit sigAddComponent(cData->componentName());
          emit sigAddLoggedValue(QVector<int>{1}, cData->entityId(), cData->componentName(), cData->newValue(), QDateTime::currentDateTime());
          retVal = true;
        }
      }
    }
    return retVal;
  }

  void DatabaseLogger::startBatchTimer()
  {
    if(m_dPtr->m_batchedExecutionTimer.isActive() == false)
    {
      m_dPtr->m_batchedExecutionTimer.start();
    }
  }
}
