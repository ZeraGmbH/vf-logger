#include "vl_sqlitedb.h"
#include <QMetaType>
#include <QDebug>
#include <QJsonDocument>
#include <QtSql>
#include <QMultiMap>

namespace VeinLogger
{

  class DBPrivate
  {
    DBPrivate(SQLiteDB *t_qPtr) : m_qPtr(t_qPtr)
    {

    }

    QHash<QString, int> m_recordIds;
    QVector<int> m_entityIds;
    QHash<QString, int> m_componentIds;

    QVector<SQLBatchData> m_batchVector;

    QFile m_queryReader;

    //commonly used queries
    QSqlQuery m_valueMapInsertQuery;
    QSqlQuery m_valueMapSequenceQuery;
    QSqlQuery m_recordMappingInsertQuery;
    QSqlQuery m_componentInsertQuery;
    QSqlQuery m_componentSequenceQuery;
    QSqlQuery m_entityInsertQuery;
    QSqlQuery m_recordInsertQuery;
    QSqlQuery m_recordSequenceQuery;

    int m_valueMapQueryCounter=0;

    QSqlDatabase m_logDB;

    SQLiteDB *m_qPtr=0;

    friend class SQLiteDB;
  };

  SQLiteDB::SQLiteDB(QObject *t_parent) : QObject(t_parent), m_dPtr(new DBPrivate(this))
  {
    connect(this, &SQLiteDB::sigDatabaseError, [](const QString &t_error){
      qWarning() << t_error;
    });
  }

  SQLiteDB::~SQLiteDB()
  {
    m_dPtr->m_logDB.close();
    delete m_dPtr;
  }

  bool SQLiteDB::hasEntityId(int t_entityId) const
  {
    return m_dPtr->m_entityIds.contains(t_entityId);
  }

  bool SQLiteDB::hasComponentName(const QString &t_componentName) const
  {
    return m_dPtr->m_componentIds.contains(t_componentName);
  }

  bool SQLiteDB::hasRecordName(const QString &t_recordName) const
  {
    return m_dPtr->m_recordIds.contains(t_recordName);
  }

  bool SQLiteDB::databaseIsOpen() const
  {
    return m_dPtr->m_logDB.isOpen();
  }

  QString SQLiteDB::databasePath() const
  {
    return m_dPtr->m_logDB.databaseName();
  }

  void SQLiteDB::initLocalData()
  {
    QSqlQuery componentQuery("SELECT * FROM components;");
    QSqlQuery entityQuery("SELECT * FROM entities;");
    QSqlQuery recordQuery("SELECT * FROM records;");

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

  void SQLiteDB::addComponent(const QString &t_componentName)
  {
    if(m_dPtr->m_componentIds.contains(t_componentName) == false)
    {
      int nextComponentId=0;

      if(m_dPtr->m_componentSequenceQuery.exec() == true)
      {
        m_dPtr->m_componentSequenceQuery.next();
        nextComponentId = m_dPtr->m_componentSequenceQuery.value(0).toInt()+1;
      }
      else
      {
        emit sigDatabaseError(QString("SQLiteDB::addComponent m_componentSequenceQuery failed: %1").arg(m_dPtr->m_componentSequenceQuery.lastError().text()));
        Q_ASSERT(false);
      }

      m_dPtr->m_componentInsertQuery.bindValue(":component_id", nextComponentId);
      m_dPtr->m_componentInsertQuery.bindValue(":component_name", t_componentName);
      if(m_dPtr->m_componentInsertQuery.exec() == false)
      {
        emit sigDatabaseError(QString("SQLiteDB::addComponent m_componentQuery failed: %1").arg(m_dPtr->m_componentInsertQuery.lastError().text()));
        Q_ASSERT(false);
      }


      m_dPtr->m_recordSequenceQuery.finish();

      if(nextComponentId > 0)
      {
        m_dPtr->m_componentIds.insert(t_componentName, nextComponentId);
      }
      else
      {
        emit sigDatabaseError(QString("Error in SQLiteDB::addComponent transaction: %1").arg(m_dPtr->m_logDB.lastError().text()));
        Q_ASSERT(false);
      }
    }
  }

  void SQLiteDB::addEntity(int t_entityId, QString t_entityName)
  {
    if(m_dPtr->m_entityIds.contains(t_entityId) == false)
    {
      m_dPtr->m_entityInsertQuery.bindValue(":entity_id", t_entityId);
      m_dPtr->m_entityInsertQuery.bindValue(":entity_name", t_entityName);

      if(m_dPtr->m_entityInsertQuery.exec() == true)
      {
        m_dPtr->m_entityIds.append(t_entityId);
      }
      else
      {
        emit sigDatabaseError(QString("(VeinLogger) SQLiteDB::addEntity m_entityInsertQuery failed: %1").arg(m_dPtr->m_entityInsertQuery.lastError().text()));
        Q_ASSERT(false);
      }
    }
  }

  int SQLiteDB::addRecord(const QString &t_recordName)
  {
    int retVal = -1;
    if(m_dPtr->m_recordIds.contains(t_recordName) == false)
    {
      int nextRecordId = 0;

      if(m_dPtr->m_recordSequenceQuery.exec() == true)
      {
        m_dPtr->m_recordSequenceQuery.next();
        nextRecordId = m_dPtr->m_recordSequenceQuery.value(0).toInt()+1;
      }
      else
      {
        emit sigDatabaseError(QString("(VeinLogger) SQLiteDB::addRecord m_recordSequenceQuery failed: %1").arg(m_dPtr->m_recordSequenceQuery.lastError().text()));
        Q_ASSERT(false);
      }

      m_dPtr->m_recordInsertQuery.bindValue(":record_id", nextRecordId);
      m_dPtr->m_recordInsertQuery.bindValue(":record_name", t_recordName);
      if(m_dPtr->m_recordInsertQuery.exec() == false)
      {
        emit sigDatabaseError(QString("(VeinLogger) SQLiteDB::addRecord m_recordQuery failed: %1").arg(m_dPtr->m_recordInsertQuery.lastError().text()));
        Q_ASSERT(false);
      }



      m_dPtr->m_recordSequenceQuery.finish();

      if(nextRecordId > 0)
      {
        m_dPtr->m_recordIds.insert(t_recordName, nextRecordId);
        retVal = nextRecordId;
      }
      else
      {
        emit sigDatabaseError(QString("(VeinLogger) Error in SQLiteDB::addRecord transaction: %1").arg(m_dPtr->m_logDB.lastError().text()));
        Q_ASSERT(false);
      }
    }
    return retVal;
  }

  void SQLiteDB::addLoggedValue(QVector<int> t_recordIds, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp)
  {
    const int componentId = m_dPtr->m_componentIds.value(t_componentName, 0);

    VF_ASSERT(m_dPtr->m_logDB.isOpen() == true, "(VeinLogger) Database is not open");

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
  }

  void SQLiteDB::addLoggedValue(QVector<QString> t_recordNames, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp)
  {
    QVector<int> recordIds;
    for(const QString recordName : t_recordNames)
    {
      if(m_dPtr->m_recordIds.contains(recordName))
      {
        recordIds.append(m_dPtr->m_recordIds.value(recordName));
      }
      else
      {
        int newRecord = addRecord(recordName);
        Q_ASSERT(newRecord >= 0);
        recordIds.append(newRecord);
      }
    }


    if(recordIds.isEmpty() == false)
    {
      addLoggedValue(recordIds, t_entityId, t_componentName, t_value, t_timestamp);
    }
  }

  bool SQLiteDB::openDatabase(const QString &t_dbPath)
  {
    //VF_ASSERT(m_dPtr->m_logDB.isOpen() == false, "(VeinLogger) Database is already open");

    if(m_dPtr->m_logDB.isOpen())
    {
      m_dPtr->m_logDB.close();
      QSqlDatabase::removeDatabase(m_dPtr->m_logDB.databaseName());
    }


    bool retVal = false;
    QSqlError dbError;

    m_dPtr->m_logDB = QSqlDatabase::addDatabase("QSQLITE"); //default database
    m_dPtr->m_logDB.setDatabaseName(t_dbPath);
    if (!m_dPtr->m_logDB.open())
    {
      dbError = m_dPtr->m_logDB.lastError();
      m_dPtr->m_logDB = QSqlDatabase();
      QSqlDatabase::removeDatabase(m_dPtr->m_logDB.databaseName());
    }

    if(dbError.type() != QSqlError::NoError)
    {
      emit sigDatabaseError(QString("(VeinLogger) Database connection failed error: %1").arg(dbError.text()));
      Q_ASSERT(false);
    }
    else
    {
      retVal = true;


      //the database was not open when these queries were initialized
      m_dPtr->m_valueMapInsertQuery = QSqlQuery(m_dPtr->m_logDB);
      m_dPtr->m_valueMapSequenceQuery = QSqlQuery(m_dPtr->m_logDB);
      m_dPtr->m_recordMappingInsertQuery = QSqlQuery(m_dPtr->m_logDB);
      m_dPtr->m_componentInsertQuery = QSqlQuery(m_dPtr->m_logDB);
      m_dPtr->m_componentSequenceQuery = QSqlQuery(m_dPtr->m_logDB);
      m_dPtr->m_entityInsertQuery = QSqlQuery(m_dPtr->m_logDB);
      m_dPtr->m_recordInsertQuery = QSqlQuery(m_dPtr->m_logDB);
      m_dPtr->m_recordSequenceQuery = QSqlQuery(m_dPtr->m_logDB);

      //setup database if necessary
      QSqlQuery schemaVersionQuery(m_dPtr->m_logDB);

      if(schemaVersionQuery.exec("pragma schema_version;") == true) //check if the file is valid (empty or a valid database)
      {
        schemaVersionQuery.first();
        if(schemaVersionQuery.value(0) == 0) //if there is no database schema or if the file does not exist, then this will create the database and initialize the schema
        {
          m_dPtr->m_queryReader.setFileName("://sqlite/schema_test_sqlite.sql");
          m_dPtr->m_queryReader.open(QFile::ReadOnly | QFile::Text);
          QTextStream queryStreamIn(&m_dPtr->m_queryReader);
          QStringList commandQueue = queryStreamIn.readAll().split(";");
          m_dPtr->m_queryReader.close();

          for(const QString &tmpCommand : commandQueue)
          {
            QSqlQuery tmpQuery(m_dPtr->m_logDB);

            if(tmpQuery.exec(tmpCommand) == false)
            {
              //ignore warnings as sqlite throws warnings for comments and empty statements
              //qWarning() << "(VeinLogger) Error executing schema query:" << tmpQuery.lastQuery() << tmpQuery.lastError();
            }
          }
        }
        schemaVersionQuery.finish();
        //prepare common queries
        /* valuemap_id INTEGER PRIMARY KEY,
         entity_id INTEGER REFERENCES entities(entity_id) NOT NULL,
         component_id INTEGER REFERENCES components(component_id) NOT NULL,
         value_timestamp VARCHAR(255) NOT NULL, -- timestamp in ISO 8601 format, example: 2016-10-06 11:58:34.504319
         component_value NUMERIC) WITHOUT ROWID; -- can be any type but numeric is preferred
      */
        m_dPtr->m_valueMapInsertQuery.prepare("INSERT INTO valuemap VALUES (?, ?, ?, ?, ?);");
        //executed to get the next id for internal tracking, other database clients must not alter the value while the internal reference is kept
        m_dPtr->m_valueMapSequenceQuery.prepare("SELECT MAX(valuemap_id) FROM valuemap;");
        m_dPtr->m_componentInsertQuery.prepare("INSERT INTO components (component_id, component_name) VALUES (:component_id, :component_name);");
        m_dPtr->m_componentSequenceQuery.prepare("SELECT MAX(component_id) FROM components;");
        m_dPtr->m_entityInsertQuery.prepare("INSERT INTO entities VALUES (:entity_id, :entity_name);");
        /* -- The record is a series of values collected over a variable duration connected to customer data
           CREATE TABLE records (record_id INTEGER PRIMARY KEY, record_name VARCHAR(255) NOT NULL UNIQUE) WITHOUT ROWID;
        */
        m_dPtr->m_recordInsertQuery.prepare("INSERT INTO records (record_id, record_name) VALUES (:record_id, :record_name);");
        //executed after the record was added to get the last used number
        m_dPtr->m_recordSequenceQuery.prepare("SELECT MAX(record_id) FROM records;");
        m_dPtr->m_recordMappingInsertQuery.prepare("INSERT INTO recordmapping VALUES (?, ?);"); //record_id, valuemap_id

        //get next valuemap_id
        if(m_dPtr->m_valueMapSequenceQuery.exec() == false)
        {
          emit sigDatabaseError(QString("(VeinLogger) Error executing m_valueMappingSequenceQuery: %1").arg(m_dPtr->m_valueMapSequenceQuery.lastError().text()));
          Q_ASSERT(false);
        }
        m_dPtr->m_valueMapSequenceQuery.next();
        m_dPtr->m_valueMapQueryCounter = m_dPtr->m_valueMapSequenceQuery.value(0).toInt()+1;
        //close the query as we read all data from it and it has to be closed to commit the transaction
        m_dPtr->m_valueMapSequenceQuery.finish();

        initLocalData();
        emit sigDatabaseReady();
      }
      else //file is not a database so we don't want to touch it
      {
        emit sigDatabaseError(QString("Unable to open database: %1\nError: %2").arg(t_dbPath).arg(schemaVersionQuery.lastError().text()));
      }
    }

    return retVal;
  }

  void SQLiteDB::runBatchedExecution()
  {
    if(m_dPtr->m_logDB.isOpen())
    {
      //addBindValue requires QList<QVariant>
      QList<QVariant> tmpValuemapIds;
      QList<QVariant> tmpEntityIds;
      QList<QVariant> tmpComponentIds;
      QList<QVariant> tmpTimestamps;
      //do not use QHash here, the sorted key lists are required
      QMultiMap<QVariant, QVariant> tmpRecordIds; //record_id, valuemap_id
      QMap<int, QVariant> tmpValues; //valuemap_id, component_value

      for(const SQLBatchData entry : m_dPtr->m_batchVector)
      {
        tmpValuemapIds.append(m_dPtr->m_valueMapQueryCounter);
        tmpEntityIds.append(entry.entityId);
        tmpComponentIds.append(entry.componentId);
        //one value can be logged to multiple records simultaneously
        for(const int currentRecordId : entry.recordIds)
        {
          tmpRecordIds.insert(currentRecordId, m_dPtr->m_valueMapQueryCounter);
        }
        tmpTimestamps.append(entry.timestamp);

        switch(static_cast<QMetaType::Type>(entry.value.type())) //see http://stackoverflow.com/questions/31290606/qmetatypefloat-not-in-qvarianttype
        {
          case QMetaType::Float:
          case QMetaType::Double:
          case QMetaType::Int:
          case QMetaType::UInt:
          case QMetaType::Long:
          case QMetaType::QString:
          {
            tmpValues.insert(m_dPtr->m_valueMapQueryCounter, entry.value);
            break;
          }
          case QMetaType::QByteArray:
          {
            tmpValues.insert(m_dPtr->m_valueMapQueryCounter, QString::fromUtf8(entry.value.value<QByteArray>()));
            break;
          }
          default:
          {
            int tmpDataType = QMetaType::type(entry.value.typeName());

            if(tmpDataType == QMetaType::type("QList<double>")) //store as double list
            {
              tmpValues.insert(m_dPtr->m_valueMapQueryCounter, convertDoubleArrayToString(entry.value));
            }
#if 0
            else if(entry.value.canConvert(QMetaType::QString) && entry.value.toString().isEmpty() == false) //last resort try to store as string
            {
              tmpStringValues.insert(nextValuemapId, entry.value.toString());
            }
#endif
            else
            {
              emit sigDatabaseError(QString("(VeinLogger) Datatype cannot be stored in DB Entity: %1 Component: %2 Type: %3")
                                    .arg(entry.entityId)
                                    .arg(m_dPtr->m_componentIds.key(entry.componentId))
                                    .arg(entry.value.typeName()));
            }
            break;
          }
        }

        ++m_dPtr->m_valueMapQueryCounter;
      }
      if(m_dPtr->m_logDB.transaction() == true)
      {
        //valuemap_id, entity_id, component_id, value_timestamp
        m_dPtr->m_valueMapInsertQuery.addBindValue(tmpValuemapIds);
        m_dPtr->m_valueMapInsertQuery.addBindValue(tmpEntityIds);
        m_dPtr->m_valueMapInsertQuery.addBindValue(tmpComponentIds);
        m_dPtr->m_valueMapInsertQuery.addBindValue(tmpTimestamps);
        m_dPtr->m_valueMapInsertQuery.addBindValue(tmpValues.values());
        if(m_dPtr->m_valueMapInsertQuery.execBatch() == false)
        {
          emit sigDatabaseError(QString("(VeinLogger) Error executing m_valueMapInsertQuery: %1").arg(m_dPtr->m_valueMapInsertQuery.lastError().text()));
          Q_ASSERT(false);
        }
        //record_id, valuemap_id
        m_dPtr->m_recordMappingInsertQuery.addBindValue(tmpRecordIds.keys());
        m_dPtr->m_recordMappingInsertQuery.addBindValue(tmpRecordIds.values());
        if(m_dPtr->m_recordMappingInsertQuery.execBatch() == false)
        {
          emit sigDatabaseError(QString("(VeinLogger) Error executing m_recordMappingQuery: %1").arg(m_dPtr->m_recordMappingInsertQuery.lastError().text()));
          Q_ASSERT(false);
        }

        if(tmpValuemapIds.isEmpty() == false)
        {
          qDebug() << "(VeinLogger) Batched" << tmpValuemapIds.length() << "queries";
        }

        if(m_dPtr->m_logDB.commit() == false) //do not use assert here, asserts are no-ops in release code
        {
          emit sigDatabaseError(QString("(VeinLogger) Error in database transaction commit: %1").arg(m_dPtr->m_logDB.lastError().text()));
          Q_ASSERT(false);
        }
      }
      else
      {
        emit sigDatabaseError(QString("(VeinLogger) Error in database transaction: %1").arg(m_dPtr->m_logDB.lastError().text()));
        Q_ASSERT(false);
      }
      m_dPtr->m_batchVector.clear();
    }
  }

  QString SQLiteDB::convertDoubleArrayToString(QVariant t_value)
  {
    QString doubleListValue;
    const QList<double> tmpDoubleList = t_value.value<QList<double>>();

    QStringList tmpResult;
    for( const double var : tmpDoubleList )
    {
      tmpResult.append(QString::number(var));
    }
    doubleListValue = QString("%1").arg(tmpResult.join(';'));

    return doubleListValue;
  }
} // namespace VeinLogger