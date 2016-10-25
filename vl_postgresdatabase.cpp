#include "vl_postgresdatabase.h"
#include <QMetaType>
#include <QDebug>
#include <QJsonDocument>

namespace VeinLogger
{
  PostgresDatabase::PostgresDatabase(QObject *t_parent) : QObject(t_parent)
  {
    m_batchTimer.setInterval(250);
    m_batchTimer.setSingleShot(true);
    connect(&m_batchTimer, &QTimer::timeout, this, &PostgresDatabase::runBatchedExecution);
  }

  bool PostgresDatabase::connectToDatabase()
  {
    bool retVal = false;
    QSqlError err;

    /// @todo replace with real login
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

      QSqlQuery bazQuery;
      bazQuery.prepare("INSERT INTO values_double_array VALUES (unnest({100000, 100001, 100002}}), unnest(:bar));");
      bazQuery.bindValue(":bar", "{2.4, 2.5, 2.6}");
      bazQuery.exec();
      qDebug() << bazQuery.executedQuery() << bazQuery.lastError() << m_logDB.driver()->hasFeature(QSqlDriver::BatchOperations);



      //the database was not open when these queries were initialized
      m_valueMappingQuery = QSqlQuery(m_logDB);
      m_valueMappingSequenceQuery = QSqlQuery(m_logDB);
      m_valueMappingSequenceSetterQuery = QSqlQuery(m_logDB);
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
      m_valueMappingQuery.prepare("INSERT INTO valuemapping VALUES (?, ?, ?, ?);"); //valuemap_id, entity_id, component_id, value_timestamp
      m_valueMappingSequenceQuery.prepare("SELECT nextval('valuemapping_valuemap_id_seq'::regclass);");
      m_valueMappingSequenceSetterQuery.prepare("SELECT setval('valuemapping_valuemap_id_seq'::regclass, :nextValuemapId);");
      m_recordMappingQuery.prepare("INSERT INTO recordmapping VALUES (?, ?);"); //record_id, valuemap_id
      m_valuesDoubleQuery.prepare("INSERT INTO values_double VALUES (?, ?);"); //valuemap_id. component_value
      m_valuesIntQuery.prepare("INSERT INTO values_int VALUES (?, ?);"); //valuemap_id. component_value
      m_valuesStringQuery.prepare("INSERT INTO values_string VALUES (?, ?);"); //valuemap_id. component_value
      m_valuesDoubleListQuery.prepare("INSERT INTO values_double_array VALUES (?, ?);"); //valuemap_id. component_value
      m_componentQuery.prepare("INSERT INTO components (name) VALUES (:componentName);");
      m_componentSequenceQuery.prepare("SELECT currval('components_id_seq'::regclass);");
      m_entityQuery.prepare("INSERT INTO entities VALUES (:entityId, :entityName);");
      m_recordQuery.prepare("INSERT INTO records (name) VALUES (:recordName);");
      m_recordSequenceQuery.prepare("SELECT currval('records_id_seq'::regclass);");

      //get next valuemap_id
      if(m_valueMappingSequenceQuery.exec() == false)
      {
        qWarning() << "(VeinLogger) Error executing m_valueMappingSequenceQuery:" << m_valueMappingSequenceQuery.lastError();
      }
      m_valueMappingSequenceQuery.next();
      m_valueMappingQueryCounter = m_valueMappingSequenceQuery.value(0).toInt();
      //close the query as we read all data from it and it has to be closed to commit the transaction
      m_valueMappingSequenceQuery.finish();

      initLocalData();

      /// @todo remove testdata
      addRecord("Testrecord");
    }

    return retVal;
  }

  void PostgresDatabase::addLoggedValue(QVector<int> t_recordIds, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp)
  {
    int componentId = m_componentIds.value(t_componentName);

    VF_ASSERT(m_logDB.isOpen() == true, "VeinLogger) Database is not open");

    //make sure the ids exist
    VF_ASSERT(componentId > 0, QStringC(QString("(VeinLogger) Unknown componentName: %1").arg(t_componentName)));
    for(int tmpRecord : t_recordIds)
    {
      VF_ASSERT(m_recordIds.key(tmpRecord).isEmpty() == false , QStringC(QString("(VeinLogger) Unknown recordId: %1").arg(tmpRecord)));
    }
    VF_ASSERT(m_entityIds.contains(t_entityId) == true, QStringC(QString("(VeinLogger) Unknown entityId: %1").arg(t_entityId)));

    SQLBatchData batchData;
    batchData.recordIds=t_recordIds;
    batchData.entityId=t_entityId;
    batchData.componentId=m_componentIds.value(t_componentName);
    batchData.value=t_value;
    batchData.timestamp=t_timestamp;

    m_batchVector.append(batchData);
    if(m_batchTimer.isActive() == false)
    {
      m_batchTimer.start();
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

      if(m_componentSequenceQuery.exec() == true)
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

  void PostgresDatabase::addEntity(int t_entityId)
  {
    if(m_entityIds.contains(t_entityId) == false)
    {
      m_entityQuery.bindValue(":entityId", t_entityId);
      m_entityQuery.bindValue(":entityName", "EMPTY");

      if(m_entityQuery.exec() == true)
      {
        m_entityIds.append(t_entityId);
      }
      else
      {
        qWarning() << "(VeinLogger) Error in PostgresDatabase::addEntity transaction:" << m_entityQuery.lastQuery() << m_logDB.lastError().text();
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

      if(m_recordSequenceQuery.exec() == true)
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

    for(SQLBatchData entry : m_batchVector)
    {
      tmpValuemapIds.append(m_valueMappingQueryCounter);
      tmpEntityIds.append(entry.entityId);
      tmpComponentIds.append(entry.componentId);
      //one value can be logged to multiple records simultaneously
      for( int currentRecordId : entry.recordIds)
      {
        tmpRecordIds.insert(currentRecordId, m_valueMappingQueryCounter);
      }
      tmpTimestamps.append(entry.timestamp);

      switch(entry.value.type())
      {
        case QMetaType::Float:
        case QMetaType::Double:
        {
          tmpDoubleValues.insert(m_valueMappingQueryCounter, entry.value);
          break;
        }
        case QMetaType::Int:
        case QMetaType::UInt:
        {
          tmpIntValues.insert(m_valueMappingQueryCounter, entry.value);
          break;
        }
        case QMetaType::QString:
        {
          tmpStringValues.insert(m_valueMappingQueryCounter, entry.value);
          break;
        }
        case QMetaType::QByteArray:
        {
          tmpStringValues.insert(m_valueMappingQueryCounter, QString::fromUtf8(entry.value.value<QByteArray>()));
          break;
        }
        default:
        {
          int tmpDataType = QMetaType::type(entry.value.typeName());
          if(tmpDataType != QMetaType::type("QList<double>"))
          {
            qDebug() << "Unrecognized datatype:" << tmpDataType << QMetaType::typeName(tmpDataType);
          }

          if(tmpDataType == QMetaType::type("QList<double>")) //store as double list
          {
            tmpDoubleArrayValues.insert(m_valueMappingQueryCounter, convertDoubleArrayToString(entry.value));
          }
          //            else if(entry.value.canConvert(QMetaType::QString) && entry.value.toString().isEmpty() == false) //last resort try to store as string
          //            {
          //              tmpStringValues.insert(nextValuemapId, entry.value.toString());
          //            }
          else
          {
            qWarning() << "(VeinLogger) Datatype cannot be stored in DB:" << entry.entityId << m_componentIds.key(entry.componentId) << entry.value;
          }
          break;
        }
      }

      ++m_valueMappingQueryCounter;
    }
    if(m_logDB.transaction() == true)
    {
      //valuemap_id, entity_id, component_id, value_timestamp
      m_valueMappingQuery.addBindValue(tmpValuemapIds);
      m_valueMappingQuery.addBindValue(tmpEntityIds);
      m_valueMappingQuery.addBindValue(tmpComponentIds);
      m_valueMappingQuery.addBindValue(tmpTimestamps);
      if(m_valueMappingQuery.execBatch() == false)
      {
        qWarning() << "(VeinLogger) Error executing m_valueMappingQuery:" << m_valueMappingQuery.lastError();
      }

      m_valueMappingSequenceSetterQuery.bindValue(":nextValuemapId", m_valueMappingQueryCounter+1);
      if(m_valueMappingSequenceSetterQuery.exec() == false)
      {
        qWarning() << "(VeinLogger) Error executing valueMappingSequenceSetterQuery:" << m_valueMappingSequenceSetterQuery.lastError();
      }

      //record_id, valuemap_id
      m_recordMappingQuery.addBindValue(tmpRecordIds.keys());
      m_recordMappingQuery.addBindValue(tmpRecordIds.values());
      if(m_recordMappingQuery.execBatch() == false)
      {
        qWarning() << "(VeinLogger) Error executing m_recordMappingQuery:" << m_recordMappingQuery.lastError();
      }

      if(tmpDoubleValues.isEmpty() == false)
      {
        //valuemap_id. component_value
        m_valuesDoubleQuery.addBindValue(tmpDoubleValues.keys());
        m_valuesDoubleQuery.addBindValue(tmpDoubleValues.values());
        if(m_valuesDoubleQuery.execBatch() == false)
        {
          qWarning() << "(VeinLogger) Error executing m_valuesDoubleQuery:" << m_valuesDoubleQuery.lastError();
        }
      }

      if(tmpDoubleArrayValues.isEmpty() == false)
      {
        //valuemap_id. component_value
        m_valuesDoubleListQuery.addBindValue(tmpDoubleArrayValues.keys());
        m_valuesDoubleListQuery.addBindValue(tmpDoubleArrayValues.values());
        if(m_valuesDoubleListQuery.execBatch() == false)
        {
          qWarning() << "(VeinLogger) Error executing m_valuesDoubleListQuery:" << m_valuesDoubleListQuery.lastError();
        }
      }

      if(tmpIntValues.isEmpty() == false)
      {
        //valuemap_id. component_value
        m_valuesIntQuery.addBindValue(tmpIntValues.keys());
        m_valuesIntQuery.addBindValue(tmpIntValues.values());
        if(m_valuesIntQuery.execBatch() == false)
        {
          qWarning() << "(VeinLogger) Error executing m_valuesIntQuery:" << m_valuesIntQuery.lastError();
        }
      }

      if(tmpStringValues.isEmpty() == false)
      {
        //valuemap_id. component_value
        m_valuesStringQuery.addBindValue(tmpStringValues.keys());
        m_valuesStringQuery.addBindValue(tmpStringValues.values());
        if(m_valuesStringQuery.execBatch() == false)
        {
          qWarning() << "(VeinLogger) Error executing m_valuesStringQuery:" << m_valuesStringQuery.lastError();
        }
      }
      qDebug() << "(VeinLogger) Batched" << tmpValuemapIds.length() << "queries";

      if(m_logDB.commit() == false) //do not use assert here, asserts are no-ops in release code
      {
        qWarning() << "(VeinLogger) Error in database transaction commit:" << m_logDB.lastError().text();
      }
    }
    else
    {
      qWarning() << "(VeinLogger) Error in database transaction:" << m_logDB.lastError().text();
    }
    m_batchVector.clear();
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
      m_entityIds.append(entityId);
    }

    while (recordQuery.next())
    {
      int recordId = recordQuery.value(0).toInt();
      QString recordName = recordQuery.value(1).toString();

      qDebug() << "(VeinLogger) Found record:" << recordId << recordName;
      m_recordIds.insert(recordName, recordId);
    }
  }

  QString PostgresDatabase::convertDoubleArrayToString(QVariant t_value)
  {
    QString doubleListValue;
    const QList<double> tmpDoubleList = t_value.value<QList<double>>();
#ifdef VL_UNREACHABLE
    m_fooStream.setString(&doubleListValue, QIODevice::WriteOnly);

    m_fooStream << '{';
    for(int i=0; i < tmpDoubleList.length()-1; ++i)
    {
      m_fooStream << tmpDoubleList.at(i);
      m_fooStream << ',';
    }
    m_fooStream << tmpDoubleList.last();
    m_fooStream << '}';
#endif

    QStringList tmpResult;
    for( const double var : tmpDoubleList )
    {
      tmpResult.append(QString::number(var));
    }
    doubleListValue = QString("{%1}").arg(tmpResult.join(','));

    return doubleListValue;
  }

} // namespace VeinLogger
