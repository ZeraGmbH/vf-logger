#include "vl_databaselogger.h"
#include "vl_sqlitedb.h"
#include "vl_datasource.h"
#include "vl_qmllogger.h"

#include <QHash>
#include <QThread>
#include <QCoreApplication>
#include <QTimer>
#include <QStorageInfo>

#include <ve_commandevent.h>
#include <vcmp_componentdata.h>
#include <vcmp_entitydata.h>
#include <vcmp_errordata.h>

namespace VeinLogger
{
  class DataLoggerPrivate
  {
    explicit DataLoggerPrivate(DatabaseLogger *t_qPtr) : m_qPtr(t_qPtr)
    {
      m_batchedExecutionTimer.setInterval(5000);
      m_batchedExecutionTimer.setSingleShot(false);
    }
    ~DataLoggerPrivate() {}

    void initOnce()
    {
      Q_ASSERT(m_initDone == false);
      if(m_initDone == false)
      {
        VeinComponent::EntityData *systemData = new VeinComponent::EntityData();
        systemData->setCommand(VeinComponent::EntityData::Command::ECMD_ADD);
        systemData->setEntityId(s_entityId);

        VeinEvent::CommandEvent *systemEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, systemData);

        emit m_qPtr->sigSendEvent(systemEvent);

        VeinComponent::ComponentData *introspectionData = nullptr;

        QHash<QString, QVariant> componentData;
        componentData.insert(s_entityNameComponentName, s_entityName);
        componentData.insert(s_loggingEnabledComponentName, QVariant(false));
        ///@todo load from persistent settings file?
        componentData.insert(s_databaseFileComponentName, QVariant(QString()));
        componentData.insert(s_filesystemDeviceComponentName, QVariant(QString()));
        componentData.insert(s_filesystemTypeComponentName, QVariant(QString()));
        componentData.insert(s_filesystemFreeComponentName, QVariant(0.0));
        componentData.insert(s_filesystemTotalComponentName, QVariant(0.0));
        componentData.insert(s_scheduledLoggingEnabledComponentName, QVariant(false));
        componentData.insert(s_scheduledLoggingBeginComponentName, QVariant(QString()));
        componentData.insert(s_scheduledLoggingDurationComponentName, QVariant(QString()));

        for(const QString &componentName : componentData.keys())
        {
          introspectionData = new VeinComponent::ComponentData();
          introspectionData->setEntityId(s_entityId);
          introspectionData->setCommand(VeinComponent::ComponentData::Command::CCMD_ADD);
          introspectionData->setComponentName(componentName);
          introspectionData->setNewValue(componentData.value(componentName));
          introspectionData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
          introspectionData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

          systemEvent = new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, introspectionData);
          emit m_qPtr->sigSendEvent(systemEvent);
        }

        m_initDone = true;
      }
    }

    //values to be recorded
    QVector<QmlLogger *> m_loggerScripts;

    SQLiteDB *m_database=0;
    DataSource *m_dataSource=0;

    DatabaseLogger *m_qPtr=0;
    QThread m_asyncDatabaseThread;
    QTimer m_batchedExecutionTimer;
    bool m_loggingEnabled=false;

    bool m_initDone=false;

    static constexpr int s_entityId = 2;
    //entity name
    static constexpr char const *s_entityName = "_LoggingSystem";
    //component names
    static constexpr char const *s_entityNameComponentName = "EntityName";
    static constexpr char const *s_loggingEnabledComponentName = "LoggingEnabled";
    static constexpr char const *s_databaseFileComponentName = "DatabaseFile";
    static constexpr char const *s_filesystemDeviceComponentName = "FilesystemDevice";
    static constexpr char const *s_filesystemTypeComponentName = "FilesystemType";
    static constexpr char const *s_filesystemFreeComponentName = "FilesystemFree";
    static constexpr char const *s_filesystemTotalComponentName = "FilesystemTotal";
    static constexpr char const *s_scheduledLoggingEnabledComponentName = "ScheduledLoggingEnabled";
    static constexpr char const *s_scheduledLoggingBeginComponentName = "ScheduledLoggingBegin";
    static constexpr char const *s_scheduledLoggingDurationComponentName = "ScheduledLoggingDuration";

    friend class DatabaseLogger;
  };

  DatabaseLogger::DatabaseLogger(SQLiteDB *t_database, DataSource *t_dataSource, QObject *t_parent) : VeinEvent::EventSystem(t_parent), m_dPtr(new DataLoggerPrivate(this))
  {
    m_dPtr->m_dataSource=t_dataSource;
    m_dPtr->m_asyncDatabaseThread.setObjectName("VFLoggerDBThread");

    m_dPtr->m_database=t_database;
    m_dPtr->m_database->moveToThread(&m_dPtr->m_asyncDatabaseThread);
    m_dPtr->m_asyncDatabaseThread.start();

    //will be queued connection due to thread affinity
    connect(this, SIGNAL(sigAddLoggedValue(QVector<QString>,int,QString,QVariant,QDateTime)), m_dPtr->m_database, SLOT(addLoggedValue(QVector<QString>,int,QString,QVariant,QDateTime)));
    connect(this, SIGNAL(sigAddEntity(int, QString)), m_dPtr->m_database, SLOT(addEntity(int, QString)));
    connect(this, SIGNAL(sigAddComponent(QString)), m_dPtr->m_database, SLOT(addComponent(QString)));
    connect(this, SIGNAL(sigAddRecord(QString)), m_dPtr->m_database, SLOT(addRecord(QString)));
    connect(this, SIGNAL(sigOpenDatabase(QString)), m_dPtr->m_database, SLOT(openDatabase(QString)));
    connect(&m_dPtr->m_batchedExecutionTimer, SIGNAL(timeout()), m_dPtr->m_database, SLOT(runBatchedExecution()));
    connect(this, &DatabaseLogger::sigAttached, [this](){ m_dPtr->initOnce(); });
  }

  DatabaseLogger::~DatabaseLogger()
  {
    m_dPtr->m_batchedExecutionTimer.stop();
    m_dPtr->m_asyncDatabaseThread.quit();
    m_dPtr->m_asyncDatabaseThread.wait();
  }

  void DatabaseLogger::addScript(QmlLogger *t_script)
  {
    if(m_dPtr->m_loggerScripts.contains(t_script) == false)
    {
      m_dPtr->m_loggerScripts.append(t_script);
    }
  }

  void DatabaseLogger::removeScript(QmlLogger *t_script)
  {
    m_dPtr->m_loggerScripts.removeAll(t_script);
  }

  bool DatabaseLogger::loggingEnabled() const
  {
    return m_dPtr->m_loggingEnabled;
  }

  int DatabaseLogger::entityId()
  {
    return DataLoggerPrivate::s_entityId;
  }

  bool DatabaseLogger::processEvent(QEvent *t_event)
  {
    using namespace VeinEvent;
    using namespace VeinComponent;

    bool retVal = false;

    if(m_dPtr->m_loggingEnabled && t_event->type()==CommandEvent::eventType())
    {
      CommandEvent *cEvent = nullptr;
      EventData *evData = nullptr;
      cEvent = static_cast<CommandEvent *>(t_event);
      Q_ASSERT(cEvent != nullptr);

      evData = cEvent->eventData();
      Q_ASSERT(evData != nullptr);

      if(cEvent->eventSubtype() == CommandEvent::EventSubtype::NOTIFICATION)
      {
        if(evData->type()==ComponentData::dataType())
        {
          ComponentData *cData=nullptr;
          cData = static_cast<ComponentData *>(evData);
          Q_ASSERT(cData != nullptr);

          QVector<QString> recordNames;
          const QVector<QmlLogger *> scripts = m_dPtr->m_loggerScripts;
          //check all scripts if they want to log the changed value
          for(const QmlLogger *entry : scripts)
          {
            if(entry->hasLoggerEntry(evData->entityId(), cData->componentName()))
            {
              recordNames.append(entry->recordName());
            }
          }
          if(recordNames.isEmpty() == false)
          {
            if(m_dPtr->m_database->hasEntityId(evData->entityId()))
            {
              emit sigAddEntity(evData->entityId(), m_dPtr->m_dataSource->getEntityName(cData->entityId()));
            }
            if(m_dPtr->m_database->hasComponentName(cData->componentName()))
            {
              emit sigAddComponent(cData->componentName());
            }
            emit sigAddLoggedValue(recordNames, cData->entityId(), cData->componentName(), cData->newValue(), QDateTime::currentDateTime());
            retVal = true;
          }
        }
      }
      else if(cEvent->eventSubtype() == CommandEvent::EventSubtype::TRANSACTION &&
              evData->entityId() == DataLoggerPrivate::s_entityId)
      {
        ComponentData *cData=nullptr;
        cData = static_cast<ComponentData *>(evData);
        Q_ASSERT(cData != nullptr);

        if(cData->componentName() == DataLoggerPrivate::s_databaseFileComponentName)
        {
          if(openDatabase(cData->newValue().toString()) == false)
          {
            retVal = true;
            VeinComponent::ErrorData *errData = new VeinComponent::ErrorData();
            errData->setEntityId(DataLoggerPrivate::s_entityId);
            errData->setOriginalData(cData);
            errData->setEventOrigin(VeinComponent::ErrorData::EventOrigin::EO_LOCAL);
            errData->setEventTarget(VeinComponent::ErrorData::EventTarget::ET_ALL);
            errData->setErrorDescription(QString("Invalid database path: %1").arg(cData->newValue().toString()));

            sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, errData));


            t_event->accept();

          }
        }
      }
    }
    return retVal;
  }

  void DatabaseLogger::setLoggingEnabled(bool t_enabled)
  {
    if(t_enabled != m_dPtr->m_loggingEnabled)
    {
      m_dPtr->m_loggingEnabled = t_enabled;
      if(t_enabled)
      {
        m_dPtr->m_batchedExecutionTimer.start();
      }
      else
      {
        m_dPtr->m_batchedExecutionTimer.stop();
      }
      VeinComponent::ComponentData *loggingEnabledCData = new VeinComponent::ComponentData();
      loggingEnabledCData->setEntityId(DataLoggerPrivate::s_entityId);
      loggingEnabledCData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
      loggingEnabledCData->setComponentName(DataLoggerPrivate::s_loggingEnabledComponentName);
      loggingEnabledCData->setNewValue(QVariant(t_enabled));
      loggingEnabledCData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
      loggingEnabledCData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

      emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, loggingEnabledCData));
    }
  }

  bool DatabaseLogger::openDatabase(const QString &t_filePath)
  {
    bool validStorage = false;
    const auto storages = QStorageInfo::mountedVolumes();
    for(const auto storDevice : storages)
    {
      if(storDevice.fileSystemType().contains("tmpfs") == false /*&& storDevice.isRoot() == false*/ && t_filePath.contains(storDevice.rootPath()))
      {
        validStorage = true;
        const double availGB = storDevice.bytesFree()/1.0e9;
        const double totalGB = storDevice.bytesTotal()/1.0e9;

        QHash<QString, QVariant> storageInfo;
        storageInfo.insert(DataLoggerPrivate::s_databaseFileComponentName, t_filePath);
        storageInfo.insert(DataLoggerPrivate::s_filesystemFreeComponentName, availGB);
        storageInfo.insert(DataLoggerPrivate::s_filesystemTotalComponentName, totalGB);
        storageInfo.insert(DataLoggerPrivate::s_filesystemDeviceComponentName, QString::fromUtf8(storDevice.device()));
        storageInfo.insert(DataLoggerPrivate::s_filesystemTypeComponentName, QString::fromUtf8(storDevice.fileSystemType()));


        VeinComponent::ComponentData *storageCData = nullptr;

        for(const QString &componentName : storageInfo.keys())
        {
          storageCData= new VeinComponent::ComponentData();
          storageCData->setEntityId(DataLoggerPrivate::s_entityId);
          storageCData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
          storageCData->setComponentName(componentName);
          storageCData->setNewValue(storageInfo.value(componentName));
          storageCData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
          storageCData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

          emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, storageCData));
        }
      }
    }

    if(validStorage == true)
    {
      emit sigOpenDatabase(t_filePath);
    }

    return validStorage;
  }
}
