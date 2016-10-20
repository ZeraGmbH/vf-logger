#include "vl_postgresdatabase.h"
#include <QMetaType>
#include <QDebug>
#include <QJsonDocument>

namespace VeinLogger
{
  PostgresDatabase::PostgresDatabase(QObject *t_parent) : QObject(t_parent)
  {

  }

  bool PostgresDatabase::connectToDatabase()
  {
    bool retVal = false;
    QSqlError err;

    m_logDB = QSqlDatabase::addDatabase("QPSQL"); //default database
    m_logDB.setDatabaseName("testdb");
    m_logDB.setHostName("127.0.0.1");
    m_logDB.setPort(15432);
    if (!m_logDB.open("testuser", "testpass"))
    {
      err = m_logDB.lastError();
      m_logDB = QSqlDatabase();
      QSqlDatabase::removeDatabase(QString("LogDB"));
    }

    if (err.type() != QSqlError::NoError)
    {
      qDebug() << "(VeinLogger) Database connection failed error:" << err.text();
    }
    else
    {
      retVal = true;
    }

    m_valueMappingQuery = QSqlQuery(m_logDB);
    m_valueMappingQuery = QSqlQuery(m_logDB);
    m_valueMappingSequenceQuery = QSqlQuery(m_logDB);
    m_recordMappingQuery = QSqlQuery(m_logDB);
    m_valuesDoubleQuery = QSqlQuery(m_logDB);
    m_valuesIntQuery = QSqlQuery(m_logDB);
    m_valuesStringQuery = QSqlQuery(m_logDB);
    m_valuesDoubleListQuery = QSqlQuery(m_logDB);
    m_componentQuery = QSqlQuery(m_logDB);
    m_componentSequenceQuery = QSqlQuery(m_logDB);
    m_entityQuery = QSqlQuery(m_logDB);
    m_recordQuery = QSqlQuery(m_logDB);
    m_recordSequenceQuery = QSqlQuery(m_logDB);

    //prepare common queries
    m_valueMappingQuery.prepare("INSERT INTO valuemapping (entity_id, component_id, value_timestamp) VALUES (:entity_id, :component_id, :value_timestamp);");
    m_valueMappingSequenceQuery.prepare("SELECT currval('valuemapping_valuemap_id_seq'::regclass);");
    m_recordMappingQuery.prepare("INSERT INTO recordmapping VALUES (:record_id, :valuemap_id);");
    m_valuesDoubleQuery.prepare("INSERT INTO values_double VALUES (:valuemap_id, :component_value);");
    m_valuesIntQuery.prepare("INSERT INTO values_int VALUES (:valuemap_id, :component_value);");
    m_valuesStringQuery.prepare("INSERT INTO values_string VALUES (:valuemap_id, :component_value);");
    m_valuesDoubleListQuery.prepare("INSERT INTO values_double_array VALUES (:valuemap_id, :component_value);");
    m_componentQuery.prepare("INSERT INTO components (name) VALUES (:componentName);");
    m_componentSequenceQuery.prepare("SELECT currval('components_id_seq'::regclass);");
    m_entityQuery.prepare("INSERT INTO entities VALUES (:entityId, :entityName);");
    m_recordQuery.prepare("INSERT INTO records (name) VALUES (:recordName);");

    /// @todo remove testdata
    addRecord("TestRecord");

    initLocalData();

    return retVal;
  }

  void PostgresDatabase::addLoggedValue(int t_recordId, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp)
  {
    int componentId = m_componentIds.value(t_componentName);

    //make sure the ids exist
    VF_ASSERT(componentId > 0, QStringC(QStringLiteral("(VeinLogger) Unknown componentName: %1").arg(t_componentName)));
    VF_ASSERT(m_recordIds.key(t_recordId).isEmpty() == false , QStringC(QStringLiteral("(VeinLogger) Unknown recordId:").arg(t_recordId)));
    VF_ASSERT(m_entityIds.key(t_entityId).isEmpty() == false, QStringC(QStringLiteral("(VeinLogger) Unknown entityId").arg(t_entityId)));

    //start a transaction since partial inserts are worthless in this case
    if(m_logDB.transaction() == true)
    {
      int valuemapId=0;

      m_valueMappingQuery.bindValue(":entity_id", t_entityId);
      m_valueMappingQuery.bindValue(":component_id", componentId);
      m_valueMappingQuery.bindValue(":value_timestamp", t_timestamp);
      if(m_valueMappingQuery.exec() == false)
      {
        qWarning() << "(VeinLogger) m_valueMappingQuery failed:" << m_logDB.lastError().text();
        m_logDB.rollback();
        return;
      }

      //returns the last inserted sequence valuemap_id from the valuemapping table

      if(m_valueMappingSequenceQuery.exec() == false)
      {
        qWarning() << "(VeinLogger) m_valueMappingSequenceQuery failed:" << m_logDB.lastError().text();
        m_logDB.rollback();
        return;
      }

      valuemapId = m_valueMappingSequenceQuery.value(0).toInt();
      m_valueMappingQuery.finish(); //required to mark query as inactive to finish the transaction

      m_recordMappingQuery.bindValue(":record_id", t_recordId);
      m_recordMappingQuery.bindValue(":valuemap_id", valuemapId);
      if(m_recordMappingQuery.exec() == false)
      {
        qWarning() << "(VeinLogger) m_valueMappingSequenceQuery failed:" << m_logDB.lastError().text();
        m_logDB.rollback();
        return;
      }

      switch(t_value.type())
      {
        //case QMetaType::Float: //currently QVariant does not know float as a type, only double... but with Qt6 this may change
        case QMetaType::Double:
        {
          m_valuesDoubleQuery.bindValue(":valuemap_id", valuemapId);
          m_valuesDoubleQuery.bindValue(":component_value", t_value);
          if(m_valuesDoubleQuery.exec() == false)
          {
            qWarning() << "(VeinLogger) m_valuesDoubleQuery failed:" << m_logDB.lastError().text();
            m_logDB.rollback();
            return;
          }
          break;
        }
        case QMetaType::Int:
        {
          m_valuesIntQuery.bindValue(":valuemap_id", valuemapId);
          m_valuesIntQuery.bindValue(":component_value", t_value);
          if(m_valuesIntQuery.exec() == false)
          {
            qWarning() << "(VeinLogger) m_valuesIntQuery failed:" << m_logDB.lastError().text();
            m_logDB.rollback();
            return;
          }
          break;
        }
        case QMetaType::QString:
        {
          m_valuesStringQuery.bindValue(":valuemap_id", valuemapId);
          m_valuesStringQuery.bindValue(":component_value", t_value);
          if(m_valuesStringQuery.exec() == false)
          {
            qWarning() << "(VeinLogger) m_valuesStringQuery failed:" << m_logDB.lastError().text();
            m_logDB.rollback();
            return;
          }
          break;
        }
        default:
        {
          if(t_value.canConvert(QMetaType::QVariantList) && t_value.toList().isEmpty() == false) //store as double list
          {
            QString tmpValue = QString("{%1}").arg(t_value.toStringList().join(","));
            m_valuesDoubleListQuery.bindValue(":valuemap_id", valuemapId);
            m_valuesDoubleListQuery.bindValue(":component_value", tmpValue);
            if(m_valuesDoubleListQuery.exec() == false)
            {
              qWarning() << "(VeinLogger) m_valuesDoubleListQuery failed:" << m_logDB.lastError().text();
              m_logDB.rollback();
              return;
            }
          }
          else if(t_value.canConvert(QMetaType::QVariantMap) && t_value.toMap().isEmpty() == false) //store map as JSON string
          {
            QJsonDocument tmpDocument = QJsonDocument::fromVariant(t_value);
            m_valuesStringQuery.bindValue(":valuemap_id", valuemapId);
            m_valuesStringQuery.bindValue(":component_value", tmpDocument.toJson());
            if(m_valuesStringQuery.exec() == false)
            {
              qWarning() << "(VeinLogger) canConvert(QMetaType::QVariantMap) on m_valuesStringQuery failed:" << m_logDB.lastError().text();
              m_logDB.rollback();
              return;
            }
          }
          else if(t_value.canConvert(QMetaType::QString) && t_value.toString().isEmpty() == false) //last resort try to store as string
          {
            m_valuesStringQuery.bindValue(":valuemap_id", valuemapId);
            m_valuesStringQuery.bindValue(":component_value", t_value.toString());
            if(m_valuesStringQuery.exec() == false)
            {
              qWarning() << "(VeinLogger) canConvert(QMetaType::QString) on m_valuesStringQuery failed:" << m_logDB.lastError().text();
              m_logDB.rollback();
              return;
            }
          }
          else
          {
            qWarning() << "(VeinLogger) cannot store type:" << t_value.typeName() << "in database";
          }
          break;
        }
      }

      if(m_logDB.commit() == false) //do not use assert here
      {
        qWarning() << "(VeinLogger) Error in database transaction commit:" << m_logDB.lastError().text();
      }
    }
    else
    {
      qWarning() << "(VeinLogger) Error in database transaction:" << m_logDB.lastError().text();
    }
  }

  void PostgresDatabase::addComponent(const QString &t_componentName)
  {
    if(m_componentIds.contains(t_componentName) == false)
    {
      int nextComponentId=0;
      m_componentQuery.bindValue(":componentName", t_componentName);
      if(m_componentQuery.exec() == false)
      {
        qWarning() << "(VeinLogger) PostgresDatabase::addComponent m_componentQuery failed";
      }

      if(m_componentSequenceQuery.exec() == false)
      {
        m_componentSequenceQuery.next();
        nextComponentId = m_componentSequenceQuery.value(0).toInt();
      }
      else
      {
        qWarning() << "(VeinLogger) PostgresDatabase::addComponent m_componentSequenceQuery failed";
      }
      m_recordSequenceQuery.finish();

      if(nextComponentId > 0)
      {
        m_componentIds.insert(t_componentName, nextComponentId);
      }
      else
      {
        qWarning() << "(VeinLogger) Error in PostgresDatabase::addComponent transaction:" << m_logDB.lastError().text();
      }
    }
  }

  void PostgresDatabase::addEntity(int t_entityId, const QString &t_entityName)
  {
    if(m_entityIds.contains(t_entityName) == false)
    {
      m_entityQuery.bindValue(":entityId", t_entityId);
      m_entityQuery.bindValue(":entityName", t_entityName);

      if(m_entityQuery.exec() == true)
      {
        m_entityIds.insert(t_entityName, t_entityId);
      }
      else
      {
        qWarning() << "(VeinLogger) Error in PostgresDatabase::addEntity transaction:" << m_logDB.lastError().text();
      }
    }
  }

  void PostgresDatabase::addRecord(const QString &t_recordName)
  {
    if(m_recordIds.contains(t_recordName) == false)
    {
      int nextRecordId = 0;
      m_recordQuery.bindValue(":recordName", t_recordName);
      if(m_recordQuery.exec() == false)
      {
        qWarning() << "(VeinLogger) PostgresDatabase::addRecord m_recordQuery failed";
      }

      if(m_recordSequenceQuery.exec("SELECT currval('records_id_seq'::regclass);") == true)
      {
        m_recordSequenceQuery.next();
        nextRecordId = m_recordSequenceQuery.value(0).toInt();
      }
      else
      {
        qWarning() << "(VeinLogger) PostgresDatabase::addRecord m_recordSequenceQuery failed";
      }
      m_recordSequenceQuery.finish();

      if(nextRecordId > 0)
      {
        m_recordIds.insert(t_recordName, nextRecordId);
      }
      else
      {
        qWarning() << "(VeinLogger) Error in PostgresDatabase::addRecord transaction:" << m_logDB.lastError().text();
      }
    }
  }

  void PostgresDatabase::initLocalData()
  {
    QSqlQuery entityQuery("SELECT * FROM entities");
    QSqlQuery componentQuery("SELECT * FROM components");
    QSqlQuery recordQuery("SELECT * FROM records");

    while (componentQuery.next())
    {
      int componentId = componentQuery.value(0).toInt();
      QString componentName = componentQuery.value(1).toString();

      qDebug() << "(VeinLogger) Found component:" << componentId << componentName;
      m_componentIds.insert(componentName, componentId);
    }

    while (entityQuery.next())
    {
      int entityId = entityQuery.value(0).toInt();
      QString entityName = entityQuery.value(1).toString();

      qDebug() << "(VeinLogger) Found entity:" << entityId << entityName;
      m_entityIds.insert(entityName, entityId);
    }

    while (recordQuery.next())
    {
      int recordId = recordQuery.value(0).toInt();
      QString recordName = recordQuery.value(1).toString();

      qDebug() << "(VeinLogger) Found record:" << recordId << recordName;
      m_recordIds.insert(recordName, recordId);
    }
  }

} // namespace VeinLogger
