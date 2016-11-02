#include "vl_postgresdatabase.h"
#include <QMetaType>
#include <QDebug>
#include <QJsonDocument>

namespace VeinLogger
{
  class PostgresDBPrivate {
    PostgresDBPrivate(PostgresDatabase *t_qPtr) : m_qPtr(t_qPtr)
    {

    }




    QHash<QString, int> m_recordIds;
    QVector<int> m_entityIds;
    QHash<QString, int> m_componentIds;

    QVector<SQLBatchData> m_batchVector;

    QTimer m_batchTimer;

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

    PostgresDatabase *m_qPtr=0;

    friend class PostgresDatabase;
  };

  PostgresDatabase::PostgresDatabase(QObject *t_parent) : QObject(t_parent)
  {
  }



  void PostgresDatabase::addComponent(const QString &t_componentName)
  {
    if(m_dPtr->m_componentIds.contains(t_componentName) == false)
    {
      int nextComponentId=0;
      m_dPtr->m_componentQuery.bindValue(":componentName", t_componentName);
      if(m_dPtr->m_componentQuery.exec() == false)
      {
        qWarning() << "(VeinLogger) PostgresDatabase::addComponent m_componentQuery failed";
      }

      if(m_dPtr->m_componentSequenceQuery.exec() == true)
      {
       m_dPtr-> m_componentSequenceQuery.next();
        nextComponentId = m_dPtr->m_componentSequenceQuery.value(0).toInt();
      }
      else
      {
        qWarning() << "(VeinLogger) PostgresDatabase::addComponent m_componentSequenceQuery failed";
      }
      m_dPtr->m_recordSequenceQuery.finish();

      if(nextComponentId > 0)
      {
        m_dPtr->m_componentIds.insert(t_componentName, nextComponentId);
      }
      else
      {
        qWarning() << "(VeinLogger) Error in PostgresDatabase::addComponent transaction:" << m_dPtr->m_logDB.lastError().text();
      }
    }
  }

  void PostgresDatabase::addEntity(int t_entityId)
  {
    if(m_dPtr->m_entityIds.contains(t_entityId) == false)
    {
      m_dPtr->m_entityQuery.bindValue(":entityId", t_entityId);
      m_dPtr->m_entityQuery.bindValue(":entityName", "EMPTY");

      if(m_dPtr->m_entityQuery.exec() == true)
      {
        m_dPtr->m_entityIds.append(t_entityId);
      }
      else
      {
        qWarning() << "(VeinLogger) Error in PostgresDatabase::addEntity transaction:" << m_dPtr->m_entityQuery.lastQuery() << m_dPtr->m_logDB.lastError().text();
      }
    }
  }

  void PostgresDatabase::addRecord(const QString &t_recordName)
  {
    if(m_dPtr->m_recordIds.contains(t_recordName) == false)
    {
      int nextRecordId = 0;
      m_dPtr->m_recordQuery.bindValue(":recordName", t_recordName);
      if(m_dPtr->m_recordQuery.exec() == false)
      {
        qWarning() << "(VeinLogger) PostgresDatabase::addRecord m_recordQuery failed";
      }

      if(m_dPtr->m_recordSequenceQuery.exec() == true)
      {
        m_dPtr->m_recordSequenceQuery.next();
        nextRecordId = m_dPtr->m_recordSequenceQuery.value(0).toInt();
      }
      else
      {
        qWarning() << "(VeinLogger) PostgresDatabase::addRecord m_recordSequenceQuery failed";
      }
      m_dPtr->m_recordSequenceQuery.finish();

      if(nextRecordId > 0)
      {
        m_dPtr->m_recordIds.insert(t_recordName, nextRecordId);
      }
      else
      {
        qWarning() << "(VeinLogger) Error in PostgresDatabase::addRecord transaction:" << m_dPtr->m_logDB.lastError().text();
      }
    }
  }

  void PostgresDatabase::addLoggedValue(QVector<int> t_recordIds, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp)
  {
    int componentId = m_dPtr->m_componentIds.value(t_componentName);

    VF_ASSERT(m_dPtr->m_logDB.isOpen() == true, "VeinLogger) Database is not open");

    //make sure the ids exist
    VF_ASSERT(componentId > 0, QStringC(QString("(VeinLogger) Unknown componentName: %1").arg(t_componentName)));
    for(int tmpRecord : t_recordIds)
    {
      VF_ASSERT(m_dPtr->m_recordIds.key(tmpRecord).isEmpty() == false , QStringC(QString("(VeinLogger) Unknown recordId: %1").arg(tmpRecord)));
    }
    VF_ASSERT(m_dPtr->m_entityIds.contains(t_entityId) == true, QStringC(QString("(VeinLogger) Unknown entityId: %1").arg(t_entityId)));

    SQLBatchData batchData;
    batchData.recordIds=t_recordIds;
    batchData.entityId=t_entityId;
    batchData.componentId=m_dPtr->m_componentIds.value(t_componentName);
    batchData.value=t_value;
    batchData.timestamp=t_timestamp;

    m_dPtr->m_batchVector.append(batchData);
    if(m_dPtr->m_batchTimer.isActive() == false)
    {
      m_dPtr->m_batchTimer.start();
    }
  }

  void PostgresDatabase::runBatchedExecution()
  {
    /// @todo complete this function HERE!

    //addBindValue requires QList<QVariant>
    QList<QVariant> tmpValuemapIds;
    QList<QVariant> tmpEntityIds;
    QList<QVariant> tmpComponentIds;
    QList<QVariant> tmpTimestamps;
    //do not use QHash here, the sorted key lists are required
    QMap<QVariant, QVariant> tmpRecordIds; //record_id, valuemap_id
    QMap<QVariant, QVariant> tmpIntValues; //valuemap_id, component_value
    QMap<QVariant, QVariant> tmpDoubleValues; //valuemap_id, component_value
    QMap<QVariant, QVariant> tmpDoubleArrayValues; //valuemap_id, component_value
    QMap<QVariant, QVariant> tmpStringValues; //valuemap_id, component_value

    for(SQLBatchData entry : m_dPtr->m_batchVector)
    {
      tmpValuemapIds.append(m_dPtr->m_valueMappingQueryCounter);
      tmpEntityIds.append(entry.entityId);
      tmpComponentIds.append(entry.componentId);
      //one value can be logged to multiple records simultaneously
      for( int currentRecordId : entry.recordIds)
      {
        tmpRecordIds.insert(currentRecordId, m_dPtr->m_valueMappingQueryCounter);
      }
      tmpTimestamps.append(entry.timestamp);

      switch(entry.value.type())
      {
        case QMetaType::Float:
        case QMetaType::Double:
        {
          tmpDoubleValues.insert(m_dPtr->m_valueMappingQueryCounter, entry.value);
          break;
        }
        case QMetaType::Int:
        case QMetaType::UInt:
        case QMetaType::Long:
        {
          tmpIntValues.insert(m_dPtr->m_valueMappingQueryCounter, entry.value);
          break;
        }
        case QMetaType::QString:
        {
          tmpStringValues.insert(m_dPtr->m_valueMappingQueryCounter, entry.value);
          break;
        }
        case QMetaType::QByteArray:
        {
          tmpStringValues.insert(m_dPtr->m_valueMappingQueryCounter, QString::fromUtf8(entry.value.value<QByteArray>()));
          break;
        }
        default:
        {
          int tmpDataType = QMetaType::type(entry.value.typeName());

          if(tmpDataType == QMetaType::type("QList<double>")) //store as double list
          {
            tmpDoubleArrayValues.insert(m_dPtr->m_valueMappingQueryCounter, convertDoubleArrayToString(entry.value));
          }
          //            else if(entry.value.canConvert(QMetaType::QString) && entry.value.toString().isEmpty() == false) //last resort try to store as string
          //            {
          //              tmpStringValues.insert(nextValuemapId, entry.value.toString());
          //            }
          else
          {
            qWarning() << "(VeinLogger) Datatype cannot be stored in DB:" << entry.entityId << m_dPtr->m_componentIds.key(entry.componentId) << entry.value;
          }
          break;
        }
      }

      ++m_dPtr->m_valueMappingQueryCounter;
    }
    if(m_dPtr->m_logDB.transaction() == true)
    {
      //valuemap_id, entity_id, component_id, value_timestamp
      m_dPtr->m_valueMappingQuery.addBindValue(tmpValuemapIds);
      m_dPtr->m_valueMappingQuery.addBindValue(tmpEntityIds);
      m_dPtr->m_valueMappingQuery.addBindValue(tmpComponentIds);
      m_dPtr->m_valueMappingQuery.addBindValue(tmpTimestamps);
      if(m_dPtr->m_valueMappingQuery.execBatch() == false)
      {
        qWarning() << "(VeinLogger) Error executing m_valueMappingQuery:" << m_dPtr->m_valueMappingQuery.lastError();
      }

      m_dPtr->m_valueMappingSequenceSetterQuery.bindValue(":nextValuemapId", m_dPtr->m_valueMappingQueryCounter+1);
      if(m_dPtr->m_valueMappingSequenceSetterQuery.exec() == false)
      {
        qWarning() << "(VeinLogger) Error executing valueMappingSequenceSetterQuery:" << m_dPtr->m_valueMappingSequenceSetterQuery.lastError();
      }

      //record_id, valuemap_id
      m_dPtr->m_recordMappingQuery.addBindValue(tmpRecordIds.keys());
      m_dPtr->m_recordMappingQuery.addBindValue(tmpRecordIds.values());
      if(m_dPtr->m_recordMappingQuery.execBatch() == false)
      {
        qWarning() << "(VeinLogger) Error executing m_recordMappingQuery:" << m_dPtr->m_recordMappingQuery.lastError();
      }

      if(tmpDoubleValues.isEmpty() == false)
      {
        //valuemap_id. component_value
        m_dPtr->m_valuesDoubleQuery.addBindValue(tmpDoubleValues.keys());
        m_dPtr->m_valuesDoubleQuery.addBindValue(tmpDoubleValues.values());
        if(m_dPtr->m_valuesDoubleQuery.execBatch() == false)
        {
          qWarning() << "(VeinLogger) Error executing m_valuesDoubleQuery:" << m_dPtr->m_valuesDoubleQuery.lastError();
        }
      }

      if(tmpDoubleArrayValues.isEmpty() == false)
      {
        //valuemap_id. component_value
        m_dPtr->m_valuesDoubleListQuery.addBindValue(tmpDoubleArrayValues.keys());
        m_dPtr->m_valuesDoubleListQuery.addBindValue(tmpDoubleArrayValues.values());
        if(m_dPtr->m_valuesDoubleListQuery.execBatch() == false)
        {
          qWarning() << "(VeinLogger) Error executing m_valuesDoubleListQuery:" << m_dPtr->m_valuesDoubleListQuery.lastError();
        }
      }

      if(tmpIntValues.isEmpty() == false)
      {
        //valuemap_id. component_value
        m_dPtr->m_valuesIntQuery.addBindValue(tmpIntValues.keys());
        m_dPtr->m_valuesIntQuery.addBindValue(tmpIntValues.values());
        if(m_dPtr->m_valuesIntQuery.execBatch() == false)
        {
          qWarning() << "(VeinLogger) Error executing m_valuesIntQuery:" << m_dPtr->m_valuesIntQuery.lastError();
        }
      }

      if(tmpStringValues.isEmpty() == false)
      {
        //valuemap_id. component_value
        m_dPtr->m_valuesStringQuery.addBindValue(tmpStringValues.keys());
        m_dPtr->m_valuesStringQuery.addBindValue(tmpStringValues.values());
        if(m_dPtr->m_valuesStringQuery.execBatch() == false)
        {
          qWarning() << "(VeinLogger) Error executing m_valuesStringQuery:" << m_dPtr->m_valuesStringQuery.lastError();
        }
      }
      qDebug() << "(VeinLogger) Batched" << tmpValuemapIds.length() << "queries";

      if(m_dPtr->m_logDB.commit() == false) //do not use assert here, asserts are no-ops in release code
      {
        qWarning() << "(VeinLogger) Error in database transaction commit:" << m_dPtr->m_logDB.lastError().text();
      }
    }
    else
    {
      qWarning() << "(VeinLogger) Error in database transaction:" << m_dPtr->m_logDB.lastError().text();
    }
    m_dPtr->m_batchVector.clear();
  }

  bool PostgresDatabase::connectToDatabase()
  {
    m_dPtr = new PostgresDBPrivate(this);
    bool retVal = false;
    QSqlError err;

    /// @todo replace with real login
    m_dPtr->m_logDB = QSqlDatabase::addDatabase("QPSQL"); //default database
    m_dPtr->m_logDB.setDatabaseName("testdb");
    m_dPtr->m_logDB.setHostName("127.0.0.1");
    m_dPtr->m_logDB.setPort(15432);
    if (!m_dPtr->m_logDB.open("testuser", "testpass"))
    {
      err = m_dPtr->m_logDB.lastError();
      m_dPtr->m_logDB = QSqlDatabase();
      QSqlDatabase::removeDatabase(QString("LogDB"));
    }

    if (err.type() != QSqlError::NoError)
    {
      qDebug() << "(VeinLogger) Database connection failed error:" << err.text();
    }
    else
    {
      retVal = true;

      //the database was not open when these queries were initialized
      m_dPtr->m_valueMappingQuery = QSqlQuery(m_dPtr->m_logDB);
      m_dPtr->m_valueMappingSequenceQuery = QSqlQuery(m_dPtr->m_logDB);
      m_dPtr->m_valueMappingSequenceSetterQuery = QSqlQuery(m_dPtr->m_logDB);
      m_dPtr->m_recordMappingQuery = QSqlQuery(m_dPtr->m_logDB);
      m_dPtr->m_valuesDoubleQuery = QSqlQuery(m_dPtr->m_logDB);
      m_dPtr->m_valuesIntQuery = QSqlQuery(m_dPtr->m_logDB);
      m_dPtr->m_valuesStringQuery = QSqlQuery(m_dPtr->m_logDB);
      m_dPtr->m_valuesDoubleListQuery = QSqlQuery(m_dPtr->m_logDB);
      m_dPtr->m_componentQuery = QSqlQuery(m_dPtr->m_logDB);
      m_dPtr->m_componentSequenceQuery = QSqlQuery(m_dPtr->m_logDB);
      m_dPtr->m_entityQuery = QSqlQuery(m_dPtr->m_logDB);
      m_dPtr->m_recordQuery = QSqlQuery(m_dPtr->m_logDB);
      m_dPtr->m_recordSequenceQuery = QSqlQuery(m_dPtr->m_logDB);

      //prepare common queries
      m_dPtr->m_valueMappingQuery.prepare("INSERT INTO valuemapping VALUES (?, ?, ?, ?);"); //valuemap_id, entity_id, component_id, value_timestamp
      m_dPtr->m_valueMappingSequenceQuery.prepare("SELECT nextval('valuemapping_valuemap_id_seq'::regclass);");
      m_dPtr->m_valueMappingSequenceSetterQuery.prepare("SELECT setval('valuemapping_valuemap_id_seq'::regclass, :nextValuemapId);");
      m_dPtr->m_recordMappingQuery.prepare("INSERT INTO recordmapping VALUES (?, ?);"); //record_id, valuemap_id
      m_dPtr->m_valuesDoubleQuery.prepare("INSERT INTO values_double VALUES (?, ?);"); //valuemap_id. component_value
      m_dPtr->m_valuesIntQuery.prepare("INSERT INTO values_int VALUES (?, ?);"); //valuemap_id. component_value
      m_dPtr->m_valuesStringQuery.prepare("INSERT INTO values_string VALUES (?, ?);"); //valuemap_id. component_value
      m_dPtr->m_valuesDoubleListQuery.prepare("INSERT INTO values_double_array VALUES (?, ?);"); //valuemap_id. component_value
      m_dPtr->m_componentQuery.prepare("INSERT INTO components (name) VALUES (:componentName);");
      m_dPtr->m_componentSequenceQuery.prepare("SELECT currval('components_id_seq'::regclass);");
      m_dPtr->m_entityQuery.prepare("INSERT INTO entities VALUES (:entityId, :entityName);");
      m_dPtr->m_recordQuery.prepare("INSERT INTO records (name) VALUES (:recordName);");
      m_dPtr->m_recordSequenceQuery.prepare("SELECT currval('records_id_seq'::regclass);");

      //get next valuemap_id
      if(m_dPtr->m_valueMappingSequenceQuery.exec() == false)
      {
        qWarning() << "(VeinLogger) Error executing m_valueMappingSequenceQuery:" << m_dPtr->m_valueMappingSequenceQuery.lastError();
      }
      m_dPtr->m_valueMappingSequenceQuery.next();
      m_dPtr->m_valueMappingQueryCounter = m_dPtr->m_valueMappingSequenceQuery.value(0).toInt();
      //close the query as we read all data from it and it has to be closed to commit the transaction
      m_dPtr->m_valueMappingSequenceQuery.finish();

      initLocalData();

      /// @todo remove testdata
      addRecord("Testrecord");
    }

    return retVal;
  }

  void PostgresDatabase::initLocalData()
  {
    QSqlQuery entityQuery("SELECT * FROM entities");
    QSqlQuery componentQuery("SELECT * FROM components");
    QSqlQuery recordQuery("SELECT * FROM records");

    m_dPtr->m_batchTimer.setInterval(30000);
    m_dPtr->m_batchTimer.setSingleShot(true);
    connect(&m_dPtr->m_batchTimer, &QTimer::timeout, this, &PostgresDatabase::runBatchedExecution);

    while (componentQuery.next())
    {
      int componentId = componentQuery.value(0).toInt();
      QString componentName = componentQuery.value(1).toString();

      qDebug() << "(VeinLogger) Found component:" << componentId << componentName;
      m_dPtr->m_componentIds.insert(componentName, componentId);
    }

    while (entityQuery.next())
    {
      int entityId = entityQuery.value(0).toInt();
      QString entityName = entityQuery.value(1).toString();

      qDebug() << "(VeinLogger) Found entity:" << entityId << entityName;
      m_dPtr->m_entityIds.append(entityId);
    }

    while (recordQuery.next())
    {
      int recordId = recordQuery.value(0).toInt();
      QString recordName = recordQuery.value(1).toString();

      qDebug() << "(VeinLogger) Found record:" << recordId << recordName;
      m_dPtr->m_recordIds.insert(recordName, recordId);
    }
  }

  QString PostgresDatabase::convertDoubleArrayToString(QVariant t_value)
  {
    QString doubleListValue;
    const QList<double> tmpDoubleList = t_value.value<QList<double>>();

    QStringList tmpResult;
    for( const double var : tmpDoubleList )
    {
      tmpResult.append(QString::number(var));
    }
    doubleListValue = QString("{%1}").arg(tmpResult.join(','));

    return doubleListValue;
  }

} // namespace VeinLogger
