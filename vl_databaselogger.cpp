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
        componentData.insert(s_loggingStatusTextComponentName, QVariant(QString("Logging inactive")));
        ///@todo load from persistent settings file?
        componentData.insert(s_databaseFileComponentName, QVariant(QString()));
        componentData.insert(s_filesystemDeviceComponentName, QVariant(QString()));
        componentData.insert(s_filesystemTypeComponentName, QVariant(QString()));
        componentData.insert(s_filesystemFreeComponentName, QVariant(0.0));
        componentData.insert(s_filesystemTotalComponentName, QVariant(0.0));
        componentData.insert(s_scheduledLoggingEnabledComponentName, QVariant(false));
        componentData.insert(s_scheduledLoggingDurationComponentName, QVariant(QString("")));

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

    void setStatusText(const QString &t_status)
    {
      if(m_loggerStatusText != t_status)
      {
        m_loggerStatusText = t_status;


        VeinComponent::ComponentData *schedulingEnabledData = new VeinComponent::ComponentData();
        schedulingEnabledData->setEntityId(DataLoggerPrivate::s_entityId);
        schedulingEnabledData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
        schedulingEnabledData->setComponentName(DataLoggerPrivate::s_loggingStatusTextComponentName);
        schedulingEnabledData->setNewValue(t_status);
        schedulingEnabledData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
        schedulingEnabledData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

        emit m_qPtr->sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, schedulingEnabledData));
      }
    }

    /**
     * @brief The logging is implemented via interpreted scripts that state which values to log
     * @see vl_qmllogger.cpp
     */
    QVector<QmlLogger *> m_loggerScripts;

    /**
     * @brief The actual database choice is an implementation detail of the DatabaseLogger
     */
    SQLiteDB *m_database=0;
    QString m_databaseFilePath;
    DataSource *m_dataSource=0;

    /**
     * @brief Qt doesn't support non blocking database access
     */
    QThread m_asyncDatabaseThread;
    /**
     * @b Logging in batches is much more efficient for SQLITE (and for spinning disk storages in general)
     * @note The batch timer is independent from the recording timeframe as it only pushes already logged values to the database
     */
    QTimer m_batchedExecutionTimer;
    bool m_loggingEnabled=false;
    bool m_scheduledLoggingEnabled=false;
    QTime m_scheduledLoggingDuration;
    QTimer m_schedulingTimer;
    bool m_initDone=false;
    QString m_loggerStatusText="Logging inactive";

    static constexpr int s_entityId = 2;
    //entity name
    static constexpr char const *s_entityName = "_LoggingSystem";
    //component names
    static constexpr char const *s_entityNameComponentName = "EntityName";
    static constexpr char const *s_loggingStatusTextComponentName = "LoggingStatus";
    static constexpr char const *s_loggingEnabledComponentName = "LoggingEnabled";
    static constexpr char const *s_databaseFileComponentName = "DatabaseFile";
    static constexpr char const *s_databaseErrorComponentName = "DatabaseError";
    static constexpr char const *s_filesystemDeviceComponentName = "FilesystemDevice";
    static constexpr char const *s_filesystemTypeComponentName = "FilesystemType";
    static constexpr char const *s_filesystemFreeComponentName = "FilesystemFree";
    static constexpr char const *s_filesystemTotalComponentName = "FilesystemTotal";
    static constexpr char const *s_scheduledLoggingEnabledComponentName = "ScheduledLoggingEnabled";
    static constexpr char const *s_scheduledLoggingDurationComponentName = "ScheduledLoggingDuration";

    DatabaseLogger *m_qPtr=0;
    friend class DatabaseLogger;
  };

  DatabaseLogger::DatabaseLogger(DataSource *t_dataSource, QObject *t_parent) : VeinEvent::EventSystem(t_parent), m_dPtr(new DataLoggerPrivate(this))
  {
    m_dPtr->m_dataSource=t_dataSource;
    m_dPtr->m_asyncDatabaseThread.setObjectName("VFLoggerDBThread");
    m_dPtr->m_schedulingTimer.setSingleShot(true);

    connect(this, &DatabaseLogger::sigAttached, [this](){ m_dPtr->initOnce(); });
    connect(&m_dPtr->m_batchedExecutionTimer, &QTimer::timeout, [this]()
    {
      if(m_dPtr->m_loggingEnabled == false)
      {
        m_dPtr->m_batchedExecutionTimer.stop();
      }
    });
    connect(&m_dPtr->m_schedulingTimer, &QTimer::timeout, [this]()
    {
      qDebug() << "m_SchedulingTimer";
      setLoggingEnabled(false);
    });
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

  void DatabaseLogger::setLoggingEnabled(bool t_enabled)
  {
    if(t_enabled != m_dPtr->m_loggingEnabled)
    {
      m_dPtr->m_loggingEnabled = t_enabled;
      if(t_enabled)
      {
        m_dPtr->m_batchedExecutionTimer.start();
        m_dPtr->setStatusText("Logging data");
        if(m_dPtr->m_scheduledLoggingEnabled)
        {
          m_dPtr->m_schedulingTimer.start();
        }
      }
      else
      {
        m_dPtr->setStatusText("Logging inactive");
        m_dPtr->m_schedulingTimer.stop();
      }
      VeinComponent::ComponentData *loggingEnabledCData = new VeinComponent::ComponentData();
      loggingEnabledCData->setEntityId(DataLoggerPrivate::s_entityId);
      loggingEnabledCData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
      loggingEnabledCData->setComponentName(DataLoggerPrivate::s_loggingEnabledComponentName);
      loggingEnabledCData->setNewValue(QVariant(t_enabled));
      loggingEnabledCData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
      loggingEnabledCData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

      emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, loggingEnabledCData));
      emit sigLoggingEnabledChanged(t_enabled);

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
        break; //there can only
      }
    }

    if(validStorage == true)
    {
      if(m_dPtr->m_database != nullptr)
      {
        m_dPtr->m_database->deleteLater();
        m_dPtr->m_database=nullptr;
      }
      m_dPtr->m_asyncDatabaseThread.quit();
      m_dPtr->m_asyncDatabaseThread.wait();
      m_dPtr->m_database=new SQLiteDB();
      m_dPtr->m_database->moveToThread(&m_dPtr->m_asyncDatabaseThread);
      m_dPtr->m_asyncDatabaseThread.start();

      //will be queued connection due to thread affinity
      connect(this, SIGNAL(sigAddLoggedValue(QVector<QString>,int,QString,QVariant,QDateTime)), m_dPtr->m_database, SLOT(addLoggedValue(QVector<QString>,int,QString,QVariant,QDateTime)));
      connect(this, SIGNAL(sigAddEntity(int, QString)), m_dPtr->m_database, SLOT(addEntity(int, QString)));
      connect(this, SIGNAL(sigAddComponent(QString)), m_dPtr->m_database, SLOT(addComponent(QString)));
      connect(this, SIGNAL(sigAddRecord(QString)), m_dPtr->m_database, SLOT(addRecord(QString)));
      connect(this, SIGNAL(sigOpenDatabase(QString)), m_dPtr->m_database, SLOT(openDatabase(QString)));
      connect(m_dPtr->m_database, SIGNAL(sigDatabaseError(QString)), this, SIGNAL(sigDatabaseError(QString)));
      connect(&m_dPtr->m_batchedExecutionTimer, SIGNAL(timeout()), m_dPtr->m_database, SLOT(runBatchedExecution()));

      m_dPtr->m_databaseFilePath = t_filePath;
      emit sigOpenDatabase(t_filePath);
    }

    return validStorage;
  }

  bool DatabaseLogger::processEvent(QEvent *t_event)
  {
    using namespace VeinEvent;
    using namespace VeinComponent;

    bool retVal = false;

    if(t_event->type()==CommandEvent::eventType())
    {
      CommandEvent *cEvent = nullptr;
      EventData *evData = nullptr;
      cEvent = static_cast<CommandEvent *>(t_event);
      Q_ASSERT(cEvent != nullptr);

      evData = cEvent->eventData();
      Q_ASSERT(evData != nullptr);

      if(m_dPtr->m_loggingEnabled && cEvent->eventSubtype() == CommandEvent::EventSubtype::NOTIFICATION)
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

          for(const QString &recName : recordNames)
          {
            if(m_dPtr->m_database->hasRecordName(recName) == false)
            {
              emit sigAddRecord(recName);
            }
          }

          if(recordNames.isEmpty() == false)
          {
            if(m_dPtr->m_database->hasEntityId(evData->entityId()) == false)
            {
              emit sigAddEntity(evData->entityId(), m_dPtr->m_dataSource->getEntityName(cData->entityId()));
            }
            if(m_dPtr->m_database->hasComponentName(cData->componentName()) == false)
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
          if(cData->newValue() != m_dPtr->m_database->databasePath() && openDatabase(cData->newValue().toString()) == false)
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
        else if(cData->componentName() == DataLoggerPrivate::s_loggingEnabledComponentName)
        {
          cEvent->accept();
          if(cData->newValue() != m_dPtr->m_loggingEnabled)
          {
            retVal = true;
            setLoggingEnabled(cData->newValue().toBool());
          }
        }
        else if(cData->componentName() == DataLoggerPrivate::s_scheduledLoggingEnabledComponentName)
        {
          cEvent->accept();
          if(cData->newValue() != m_dPtr->m_scheduledLoggingEnabled)
          {
            retVal = true;
            m_dPtr->m_scheduledLoggingEnabled = cData->newValue().toBool();

            VeinComponent::ComponentData *schedulingEnabledData = new VeinComponent::ComponentData();
            schedulingEnabledData->setEntityId(DataLoggerPrivate::s_entityId);
            schedulingEnabledData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
            schedulingEnabledData->setComponentName(DataLoggerPrivate::s_scheduledLoggingEnabledComponentName);
            schedulingEnabledData->setNewValue(cData->newValue());
            schedulingEnabledData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
            schedulingEnabledData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

            emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, schedulingEnabledData));
            setLoggingEnabled(false);
          }
        }
        else if(cData->componentName() == DataLoggerPrivate::s_scheduledLoggingDurationComponentName)
        {
          cEvent->accept();
          const QTime tmpTime = QTime::fromString(cData->newValue().toString(), "hh:mm:ss");
          if(tmpTime.isValid() == false)
          {
            VeinComponent::ErrorData *errData = new VeinComponent::ErrorData();
            errData->setEntityId(DataLoggerPrivate::s_entityId);
            errData->setOriginalData(cData);
            errData->setEventOrigin(VeinComponent::ErrorData::EventOrigin::EO_LOCAL);
            errData->setEventTarget(VeinComponent::ErrorData::EventTarget::ET_ALL);
            errData->setErrorDescription(QString("Invalid logging duration: %1").arg(cData->newValue().toString()));

            sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, errData));

            Q_ASSERT(false); //client shouldn't be able to send invalid time data
          }
          else if(tmpTime != m_dPtr->m_scheduledLoggingDuration)
          {
            const int logDurationMsecs = QTime::fromString("00:00:00", "hh:mm:ss").msecsTo(tmpTime);
            retVal = true;
            m_dPtr->m_scheduledLoggingDuration = tmpTime;
            if(logDurationMsecs > 0)
            {
              m_dPtr->m_schedulingTimer.setInterval(logDurationMsecs);
              if(m_dPtr->m_loggingEnabled)
              {
                m_dPtr->m_schedulingTimer.start(); //restart timer
              }
            }
            VeinComponent::ComponentData *schedulingEnabledData = new VeinComponent::ComponentData();
            schedulingEnabledData->setEntityId(DataLoggerPrivate::s_entityId);
            schedulingEnabledData->setCommand(VeinComponent::ComponentData::Command::CCMD_SET);
            schedulingEnabledData->setComponentName(DataLoggerPrivate::s_scheduledLoggingDurationComponentName);
            schedulingEnabledData->setNewValue(cData->newValue());
            schedulingEnabledData->setEventOrigin(VeinEvent::EventData::EventOrigin::EO_LOCAL);
            schedulingEnabledData->setEventTarget(VeinEvent::EventData::EventTarget::ET_ALL);

            emit sigSendEvent(new VeinEvent::CommandEvent(VeinEvent::CommandEvent::EventSubtype::NOTIFICATION, schedulingEnabledData));
          }
        }
      }
    }
    return retVal;
  }
}
