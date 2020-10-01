#include "vl_sqlitedb.h"
#include <QMetaType>
#include <QDebug>
#include <QJsonDocument>
#include <QtSql>
#include <QtSql/QSqlQuery>
#include <QMultiMap>

namespace VeinLogger
{

class DBPrivate
{

    DBPrivate(SQLiteDB *t_qPtr) :
        m_qPtr(t_qPtr)
    {

    }

    QVariant getTextRepresentation(const QVariant &t_value) const
    {
        QVariant retVal;
        switch(static_cast<QMetaType::Type>(t_value.type())) //see http://stackoverflow.com/questions/31290606/qmetatypefloat-not-in-qvarianttype
        {
        case QMetaType::Bool:
        case QMetaType::Float:
        case QMetaType::Double:
        case QMetaType::Int:
        case QMetaType::UInt:
        case QMetaType::Long:
        case QMetaType::LongLong:
        case QMetaType::QString:
        case QMetaType::QByteArray:
        {
            retVal = t_value;
            break;
        }
        case QMetaType::QVariant:
        {
            //try to store as string
            retVal = t_value.toString();
            break;
        }
        case QMetaType::QVariantMap:
        {
            QJsonDocument tmpDoc;
            tmpDoc.setObject(QJsonObject::fromVariantMap(t_value.toMap()));
            retVal = QString::fromUtf8(tmpDoc.toJson());
            break;
        }
        default:
        {
            const int tmpDataType = QMetaType::type(t_value.typeName());


            if(tmpDataType == QMetaType::type("QList<double>")) //store as double list
            {
                retVal = convertListToString<QList<double> >(t_value);
            }
            else if(tmpDataType == QMetaType::type("QList<int>")) //store as int list
            {
                retVal = convertListToString<QList<int> >(t_value);
            }
            else if(tmpDataType == QMetaType::type("QStringList") || tmpDataType == QMetaType::type("QList<QString>")) //store as string
            {
                retVal = t_value.toStringList().join(';');
            }
            break;
        }
        }
        return retVal;
    }

    QByteArray getBinaryRepresentation(const QVariant &t_value) const
    {
        QByteArray tmpData;
        QDataStream dataWriter(&tmpData, QIODevice::WriteOnly);
        dataWriter.device()->seek(0);
        dataWriter.setVersion(QDataStream::Qt_5_0);
        dataWriter << t_value;

        return tmpData;
    }

    template <class T> QString convertListToString(QVariant t_value) const
    {
        QString doubleListValue;
        const T tmpDoubleList = t_value.value<T>();

        QStringList tmpResult;
        for( const double var : tmpDoubleList )
        {
            tmpResult.append(QString::number(var));
        }
        doubleListValue = QString("%1").arg(tmpResult.join(';'));

        return doubleListValue;
    }

    QHash<QString, int> m_recordIds;
    QHash<int, QString> m_transactionIds;
    QVector<int> m_entityIds;
    QHash<QString, int> m_componentIds;

    QVector<SQLBatchData> m_batchVector;

    QFile m_queryReader;

    //commonly used queries
    /**
     * @brief m_valueMapInsertQuery
     * Insert values in database
     */
    QSqlQuery m_valueMapInsertQuery;
    /**
     * @brief m_valueMapSequenceQuery
     * Get highest value id in database
     */
    QSqlQuery m_valueMapSequenceQuery;
    /**
     * @brief m_transactionMappingInsertQuery
     * Add transaction to database
     *
     * The id is needed beacause writing is done in batches
     * and the ids are managed in this class parallel to the database
     */
    QSqlQuery m_transactionMappingInsertQuery;
    /**
     * @brief m_componentInsertQuery
     * Add component to database
     */
    QSqlQuery m_componentInsertQuery;
    /**
     * @brief m_componentSequenceQuery
     * get highest component id in database
     */
    QSqlQuery m_componentSequenceQuery;
    /**
     * @brief m_entityInsertQuery
     * Add entity to database
     *
     * The id is needed beacause writing is done in batches
     * and the ids are managed in this class parallel to the database
     */
    QSqlQuery m_entityInsertQuery;
    /**
     * @brief m_transactionInsertQuery
     * add transactin to database
     */
    QSqlQuery m_transactionInsertQuery;
    /**
     * @brief m_transactionSequenceQuery
     * get highest transaction id in database
     */
    QSqlQuery m_transactionSequenceQuery;
    /**
     * @brief m_recordInsertQuery
     * add record to database
     *
     * The id is needed beacause writing is done in batches
     * and the ids are managed in this class parallel to the database
     */
    QSqlQuery m_recordInsertQuery;
    /**
     * @brief m_recordSequenceQuery
     * get highest record id to database
     *
     * The id is needed beacause writing is done in batches
     * and the ids are managed in this class parallel to the database
     */
    QSqlQuery m_recordSequenceQuery;

    int m_valueMapQueryCounter=0;
    /**
     * @brief m_logDB
     * manages the actual database access
     */
    QSqlDatabase m_logDB;

    SQLiteDB::STORAGE_MODE m_storageMode=SQLiteDB::STORAGE_MODE::TEXT;

    SQLiteDB *m_qPtr=nullptr;

    friend class SQLiteDB;
};

SQLiteDB::SQLiteDB(QObject *t_parent) : AbstractLoggerDB(t_parent), m_dPtr(new DBPrivate(this))
{
    m_dPtr->m_logDB = QSqlDatabase::addDatabase("QSQLITE", "VFLogDB"); //default database
    connect(this, &SQLiteDB::sigDatabaseError, [](const QString &t_error){
        qCWarning(VEIN_LOGGER) << t_error;
    });
}

SQLiteDB::~SQLiteDB()
{
    runBatchedExecution(); //finish the remaining batch of data
    m_dPtr->m_logDB.close();
    delete m_dPtr;
    QSqlDatabase::removeDatabase("VFLogDB");
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

void SQLiteDB::setStorageMode(AbstractLoggerDB::STORAGE_MODE t_storageMode)
{
    if(m_dPtr->m_storageMode != t_storageMode)
    {
        m_dPtr->m_storageMode = t_storageMode;
    }
}

AbstractLoggerDB::STORAGE_MODE SQLiteDB::getStorageMode() const
{
    return m_dPtr->m_storageMode;
}

std::function<bool (QString)> SQLiteDB::getDatabaseValidationFunction() const
{
    return isValidDatabase;
}

bool SQLiteDB::isValidDatabase(QString t_dbPath)
{
    bool retVal = false;
    QFile dbFile(t_dbPath);

    if(dbFile.exists())
    {
        QSqlDatabase tmpDB = QSqlDatabase::addDatabase("QSQLITE", "TempDB");
        tmpDB.setConnectOptions(QLatin1String("QSQLITE_OPEN_READONLY"));

        tmpDB.setDatabaseName(t_dbPath);
        if(tmpDB.open())
        {
            QSqlQuery schemaValidationQuery(tmpDB);
            if(schemaValidationQuery.exec("SELECT name FROM sqlite_master WHERE type = 'table';"))
            {
                QSet<QString> requiredTables {"records", "entities", "components", "valuemap", "transactions"};
                QSet<QString> foundTables;
                while(schemaValidationQuery.next())
                {
                    foundTables.insert(schemaValidationQuery.value(0).toString());
                }
                schemaValidationQuery.finish();
                requiredTables.subtract(foundTables);
                if(requiredTables.isEmpty())
                {
                    retVal = true;
                }
            }
        }
        tmpDB.close();
        tmpDB = QSqlDatabase();
        QSqlDatabase::removeDatabase("TempDB");
    }
    return retVal;
}

void SQLiteDB::initLocalData()
{
    QSqlQuery componentQuery("SELECT * FROM components;", m_dPtr->m_logDB);
    QSqlQuery entityQuery("SELECT * FROM entities;", m_dPtr->m_logDB);
    QSqlQuery recordQuery("SELECT * FROM records;", m_dPtr->m_logDB);
    QSqlQuery transactionQuery("SELECT * FROM transactions;", m_dPtr->m_logDB);


    while (componentQuery.next())
    {
        int componentId = componentQuery.value(0).toInt();
        QString componentName = componentQuery.value(1).toString();

        vCDebug(VEIN_LOGGER) << "Found component:" << componentId << componentName;
        m_dPtr->m_componentIds.insert(componentName, componentId);
    }
    componentQuery.finish();

    while (entityQuery.next())
    {
        int entityId = entityQuery.value(0).toInt();
        QString entityName = entityQuery.value(1).toString();

        vCDebug(VEIN_LOGGER) << "Found entity:" << entityId << entityName;
        m_dPtr->m_entityIds.append(entityId);
    }
    entityQuery.finish();

    while (recordQuery.next())
    {
        int recordId = recordQuery.value(0).toInt();
        QString recordName = recordQuery.value(1).toString();

        vCDebug(VEIN_LOGGER) << "Found record:" << recordId << recordName;
        m_dPtr->m_recordIds.insert(recordName, recordId);
    }
    recordQuery.finish();

    while (transactionQuery.next())
    {
        int transactionId = recordQuery.value(0).toInt();
        QString transactionName = recordQuery.value(1).toString();

        vCDebug(VEIN_LOGGER) << "Found transaction:" << transactionId << transactionName;
        m_dPtr->m_transactionIds.insert(transactionId, transactionName);
    }
    transactionQuery.finish();
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

        m_dPtr->m_componentInsertQuery.bindValue(":id", nextComponentId);
        m_dPtr->m_componentInsertQuery.bindValue(":component_name", t_componentName);
        if(m_dPtr->m_componentInsertQuery.exec() == false)
        {
            emit sigDatabaseError(QString("SQLiteDB::addComponent m_componentQuery failed: %1").arg(m_dPtr->m_componentInsertQuery.lastError().text()));
            Q_ASSERT(false);
        }


        m_dPtr->m_componentSequenceQuery.finish();

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
        m_dPtr->m_entityInsertQuery.bindValue(":id", t_entityId);
        m_dPtr->m_entityInsertQuery.bindValue(":entity_name", t_entityName);

        if(m_dPtr->m_entityInsertQuery.exec() == true)
        {
            m_dPtr->m_entityIds.append(t_entityId);
        }
        else
        {
            emit sigDatabaseError(QString("SQLiteDB::addEntity m_entityInsertQuery failed: %1").arg(m_dPtr->m_entityInsertQuery.lastError().text()));
            Q_ASSERT(false);
        }
    }
}

int SQLiteDB::addTransaction(const QString &t_transactionName, const QString &t_recordName, const QString &t_contentSets)
{
    int retVal = -1;
    int recordId = 0;

    //check if record exists. If record does not exist add to list.
    if(m_dPtr->m_recordIds.contains(t_recordName))
    {
        recordId=m_dPtr->m_recordIds.value(t_recordName);
    }
    else
    {
        int newRecord = addRecord(t_recordName);
        Q_ASSERT(newRecord >= 0);
        recordId = newRecord;
    }


    int nexttransactionId = 0;

    if(m_dPtr->m_transactionSequenceQuery.exec() == true)
    {
        m_dPtr->m_transactionSequenceQuery.next();
        nexttransactionId = m_dPtr->m_transactionSequenceQuery.value(0).toInt()+1;
    }
    else
    {
        emit sigDatabaseError(QString("SQLiteDB::addTrancaction m_tranactionSequenceQuery failed: %1").arg(m_dPtr->m_transactionSequenceQuery.lastError().text()));
        Q_ASSERT(false);
    }

    m_dPtr->m_transactionInsertQuery.bindValue(":id", nexttransactionId);
    m_dPtr->m_transactionInsertQuery.bindValue(":recordsid", recordId);
    m_dPtr->m_transactionInsertQuery.bindValue(":transaction_name", t_transactionName);
    m_dPtr->m_transactionInsertQuery.bindValue(":contentset_names", t_contentSets);
    if(m_dPtr->m_transactionInsertQuery.exec() == false)
    {
        emit sigDatabaseError(QString("SQLiteDB::addTransaction m_transactionsQuery failed: %1").arg(m_dPtr->m_transactionInsertQuery.lastError().text()));
        Q_ASSERT(false);
    }



    m_dPtr->m_transactionSequenceQuery.finish();

    if(nexttransactionId > 0)
    {
        retVal = nexttransactionId;
    }
    else
    {
        emit sigDatabaseError(QString("Error in SQLiteDB::addTransaction transaction: %1").arg(m_dPtr->m_logDB.lastError().text()));
        Q_ASSERT(false);
    }

    return retVal;
}

bool SQLiteDB::addStartTime(int t_transactionId, QDateTime t_time)
{
    QSqlQuery logtimeQuery(QString("UPDATE transactions SET (start_time) = ('\%0\') WHERE id = \'%1\';").arg(t_time.toString()).arg(t_transactionId),m_dPtr->m_logDB);
    if(logtimeQuery.exec()){
        logtimeQuery.finish();
        return true;
    }

    return false;

}

bool SQLiteDB::addStopTime(int t_transactionId, QDateTime t_time)
{
    QSqlQuery logtimeQuery(QString("UPDATE transactions SET (stop_time) = ('\%0\') WHERE id = \'%1\';").arg(t_time.toString()).arg(t_transactionId),m_dPtr->m_logDB);
    if(logtimeQuery.exec()){
        logtimeQuery.finish();
        return true;
    }

    return false;
}

int SQLiteDB::addRecord(const QString &t_recordName)
{
    int retVal = -1;
    if(m_dPtr->m_recordIds.contains(t_recordName) == false)
    {
        int nextrecordId = 0;

        if(m_dPtr->m_recordSequenceQuery.exec() == true)
        {
            m_dPtr->m_recordSequenceQuery.next();
            nextrecordId = m_dPtr->m_recordSequenceQuery.value(0).toInt()+1;
        }
        else
        {
            emit sigDatabaseError(QString("SQLiteDB::addRecord m_recordSequenceQuery failed: %1").arg(m_dPtr->m_recordSequenceQuery.lastError().text()));
            Q_ASSERT(false);
        }

        m_dPtr->m_recordInsertQuery.bindValue(":id", nextrecordId);
        m_dPtr->m_recordInsertQuery.bindValue(":record_name", t_recordName);
        if(m_dPtr->m_recordInsertQuery.exec() == false)
        {
            emit sigDatabaseError(QString("SQLiteDB::addRecord m_recordQuery failed: %1").arg(m_dPtr->m_recordInsertQuery.lastError().text()));
            Q_ASSERT(false);
        }



        m_dPtr->m_recordSequenceQuery.finish();

        if(nextrecordId > 0)
        {
            m_dPtr->m_recordIds.insert(t_recordName, nextrecordId);
            retVal = nextrecordId;
            emit sigNewRecordList(QStringList(m_dPtr->m_recordIds.keys()));
        }
        else
        {
            emit sigDatabaseError(QString("Error in SQLiteDB::addRecord transaction: %1").arg(m_dPtr->m_logDB.lastError().text()));
            Q_ASSERT(false);
        }
    }
    return retVal;
}

void SQLiteDB::addLoggedValue(int t_recordId, QVector<int> t_transactionIds, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp)
{
    const int componentId = m_dPtr->m_componentIds.value(t_componentName, 0);

    VF_ASSERT(m_dPtr->m_logDB.isOpen() == true, "Database is not open");

    //make sure the ids exist
    VF_ASSERT(componentId > 0, QStringC(QString("(VeinLogger) Unknown componentName: %1").arg(t_componentName)));

    VF_ASSERT(m_dPtr->m_recordIds.key(t_recordId).isEmpty() == false , QStringC(QString("(VeinLogger) Unknown recordId: %1").arg(t_recordId)));

    VF_ASSERT(m_dPtr->m_entityIds.contains(t_entityId) == true, QStringC(QString("(VeinLogger) Unknown entityId: %1").arg(t_entityId)));

    SQLBatchData batchData;
    batchData.recordId=t_recordId;
    batchData.transactionIds=t_transactionIds;
    batchData.entityId=t_entityId;
    batchData.componentId=m_dPtr->m_componentIds.value(t_componentName);
    batchData.value=t_value;
    batchData.timestamp=t_timestamp;

    m_dPtr->m_batchVector.append(batchData);
}

void SQLiteDB::addLoggedValue(const QString &t_recordName, QVector<int> t_transactionIds, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp)
{
    int recordId = 0;

    if(m_dPtr->m_recordIds.contains(t_recordName))
    {
        recordId=m_dPtr->m_recordIds.value(t_recordName);
    }
    else
    {
        int newRecord = addRecord(t_recordName);
        Q_ASSERT(newRecord >= 0);
        recordId=newRecord;
    }
    addLoggedValue(recordId, t_transactionIds, t_entityId, t_componentName, t_value, t_timestamp);

}

bool SQLiteDB::openDatabase(const QString &t_dbPath)
{
    QFileInfo fInfo(t_dbPath);
    bool retVal = false;
    if(fInfo.absoluteDir().exists())
    {
        QSqlError dbError;
        if(m_dPtr->m_logDB.isOpen())
        {
            m_dPtr->m_logDB.close();
        }

        m_dPtr->m_logDB.setDatabaseName(t_dbPath);
        if (!m_dPtr->m_logDB.open())
        {
            dbError = m_dPtr->m_logDB.lastError();
            m_dPtr->m_logDB = QSqlDatabase();
        }

        if(dbError.type() != QSqlError::NoError)
        {
            emit sigDatabaseError(QString("Database connection failed error: %1").arg(dbError.text()));
            Q_ASSERT(false);
        }
        else
        {
            retVal = true;


            //the database was not open when these queries were initialized
            m_dPtr->m_valueMapInsertQuery = QSqlQuery(m_dPtr->m_logDB);
            m_dPtr->m_valueMapSequenceQuery = QSqlQuery(m_dPtr->m_logDB);
            m_dPtr->m_transactionMappingInsertQuery = QSqlQuery(m_dPtr->m_logDB);
            m_dPtr->m_componentInsertQuery = QSqlQuery(m_dPtr->m_logDB);
            m_dPtr->m_componentSequenceQuery = QSqlQuery(m_dPtr->m_logDB);
            m_dPtr->m_entityInsertQuery = QSqlQuery(m_dPtr->m_logDB);
            m_dPtr->m_transactionInsertQuery = QSqlQuery(m_dPtr->m_logDB);
            m_dPtr->m_transactionSequenceQuery = QSqlQuery(m_dPtr->m_logDB);
            m_dPtr->m_recordSequenceQuery = QSqlQuery(m_dPtr->m_logDB);
            m_dPtr->m_recordInsertQuery = QSqlQuery(m_dPtr->m_logDB);

            //setup database if necessary
            QSqlQuery schemaVersionQuery(m_dPtr->m_logDB);

            if(schemaVersionQuery.exec("pragma schema_version;") == true) //check if the file is valid (empty or a valid database)
            {
                schemaVersionQuery.first();
                if(schemaVersionQuery.value(0) == 0) //if there is no database schema or if the file does not exist, then this will create the database and initialize the schema
                {
                    m_dPtr->m_queryReader.setFileName("://sqlite/schema_sqlite.sql");
                    qCDebug(VEIN_LOGGER) << "No schema found in db:"<< t_dbPath << "creating schema from:" << m_dPtr->m_queryReader.fileName();
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
                            //qCWarning(VEIN_LOGGER) << "Error executing schema query:" << tmpQuery.lastQuery() << tmpQuery.lastError();
                        }
                    }
                }
                schemaVersionQuery.finish();
                //prepare common queries
                /* id INTEGER PRIMARY KEY,
           * entitiesid INTEGER REFERENCES entities(entity_id) NOT NULL,
           * componentid INTEGER REFERENCES components(component_id) NOT NULL,
           * value_timestamp VARCHAR(255) NOT NULL, -- timestamp in ISO 8601 format, example: 2016-10-06 11:58:34.504319
           * component_value NUMERIC) WITHOUT ROWID; -- can be any type but numeric is preferred
           */
                m_dPtr->m_valueMapInsertQuery.prepare("INSERT INTO valuemap VALUES (?, ?, ?, ?, ?);");
                //executed to get the next id for internal tracking, other database clients must not alter the value while the internal reference is kept
                m_dPtr->m_valueMapSequenceQuery.prepare("SELECT MAX(id) FROM valuemap;");
                m_dPtr->m_componentInsertQuery.prepare("INSERT INTO components (id, component_name) VALUES (:id, :component_name);");
                m_dPtr->m_componentSequenceQuery.prepare("SELECT MAX(id) FROM components;");
                m_dPtr->m_entityInsertQuery.prepare("INSERT INTO entities VALUES (:id, :entity_name);");
                /* -- The record is a series of values collected over a variable duration connected to customer data
           * CREATE TABLE records (id INTEGER PRIMARY KEY, record_name VARCHAR(255) NOT NULL UNIQUE) WITHOUT ROWID;
           */
                m_dPtr->m_transactionInsertQuery.prepare("INSERT INTO transactions (id, recordsid, transaction_name, contentset_names, start_time, stop_time) VALUES (:id, :recordsid, :transaction_name, :contentset_names, :start_time, :stop_time);");
                //executed after the transactions was added to get the last used number
                m_dPtr->m_transactionSequenceQuery.prepare("SELECT MAX(id) FROM transactions;");
                m_dPtr->m_transactionMappingInsertQuery.prepare("INSERT INTO transactions_valuemap VALUES (?, ?);"); //recordid, valuemapid

                m_dPtr->m_recordInsertQuery.prepare("INSERT INTO records (id, record_name) VALUES (:id, :record_name);");
                //ecexute after record was added to  get last used number
                m_dPtr->m_recordSequenceQuery.prepare("SELECT MAX(id) FROM records");



                //get next valuemap_id
                if(m_dPtr->m_valueMapSequenceQuery.exec() == false)
                {
                    emit sigDatabaseError(QString("Error executing m_valueMappingSequenceQuery: %1").arg(m_dPtr->m_valueMapSequenceQuery.lastError().text()));
                    Q_ASSERT(false);
                }
                m_dPtr->m_valueMapSequenceQuery.next();
                m_dPtr->m_valueMapQueryCounter = m_dPtr->m_valueMapSequenceQuery.value(0).toInt()+1;
                //close the query as we read all data from it and it has to be closed to commit the transaction
                m_dPtr->m_valueMapSequenceQuery.finish();




                initLocalData();
                emit sigNewRecordList(QStringList(m_dPtr->m_recordIds.keys()));

                QSqlQuery tuningQuery(m_dPtr->m_logDB);
                tuningQuery.exec("pragma journal_mode = memory;"); //prevent .journal files and speed up the logging

                emit sigDatabaseReady();
            }
            else //file is not a database so we don't want to touch it
            {
                emit sigDatabaseError(QString("Unable to open database: %1\nError: %2").arg(t_dbPath).arg(schemaVersionQuery.lastError().text()));
            }
        }
    }
    else
    {
        emit sigDatabaseError(QString("Error accessing database in directory: %1\nError: directory does not exist").arg(fInfo.absoluteDir().absolutePath()));
    }

    return retVal;
}



void SQLiteDB::runBatchedExecution()
{
    if(m_dPtr->m_logDB.isOpen())
    {
        //addBindValue requires QList<QVariant>
        QList<QVariant> tmpTimestamps;
        QList<QVariant> tmpComponentIds;
        QList<QVariant> tmpValuemapIds;
        QList<QVariant> tmpEntityIds;

        //do not use QHash here, the sorted key lists are required
        QMultiMap<QVariant, QVariant> tmpTransactionIds; //record_id, valuemap_id
        QSet<int> activeTransactions;
        QMap<int, QVariant> tmpValues; //valuemap_id, component_value

        //code that is used in both loops
        const auto commonCode = [&](const SQLBatchData &entry) {
            tmpValuemapIds.append(m_dPtr->m_valueMapQueryCounter);
            tmpEntityIds.append(entry.entityId);
            tmpComponentIds.append(entry.componentId);
            //one value can be logged to multiple records simultaneously
            for(const int currentTransId : entry.transactionIds)
            {
                tmpTransactionIds.insert(currentTransId, m_dPtr->m_valueMapQueryCounter);
                activeTransactions.insert(currentTransId);
            }
            tmpTimestamps.append(entry.timestamp);
            ++m_dPtr->m_valueMapQueryCounter;
        };

        if(m_dPtr->m_storageMode == SQLiteDB::STORAGE_MODE::TEXT)
        {
            for(const SQLBatchData &entry : qAsConst(m_dPtr->m_batchVector))
            {
                tmpValues.insert(m_dPtr->m_valueMapQueryCounter, m_dPtr->getTextRepresentation(entry.value)); //store as text
                commonCode(entry);
            }
        }
        else if(m_dPtr->m_storageMode == SQLiteDB::STORAGE_MODE::BINARY)
        {
            for(const SQLBatchData &entry : qAsConst(m_dPtr->m_batchVector))
            {
                tmpValues.insert(m_dPtr->m_valueMapQueryCounter, m_dPtr->getBinaryRepresentation(entry.value)); //store as binary
                commonCode(entry);
            }
        }

        if(m_dPtr->m_logDB.transaction() == true)
        {
            //valuemap_id, transactionid, value_timestamp, value, entity_id, component_id,
            m_dPtr->m_valueMapInsertQuery.addBindValue(tmpValuemapIds);
            m_dPtr->m_valueMapInsertQuery.addBindValue(tmpTimestamps);
            m_dPtr->m_valueMapInsertQuery.addBindValue(tmpValues.values());
            m_dPtr->m_valueMapInsertQuery.addBindValue(tmpComponentIds);
            m_dPtr->m_valueMapInsertQuery.addBindValue(tmpEntityIds);


            if(m_dPtr->m_valueMapInsertQuery.execBatch() == false)
            {
                emit sigDatabaseError(QString("Error executing m_valueMapInsertQuery: %1").arg(m_dPtr->m_valueMapInsertQuery.lastError().text()));
                Q_ASSERT(false);
            }
            //transaction_id, valuemap_id
            m_dPtr->m_transactionMappingInsertQuery.addBindValue(tmpTransactionIds.keys());
            m_dPtr->m_transactionMappingInsertQuery.addBindValue(tmpTransactionIds.values());
            if(m_dPtr->m_transactionMappingInsertQuery.execBatch() == false)
            {
                emit sigDatabaseError(QString("Error executing m_recordMappingQuery: %1").arg(m_dPtr->m_transactionMappingInsertQuery.lastError().text()));
                Q_ASSERT(false);
            }

            // Add stop time to active transactions. we have to that here becaus a bathc might be written after the script is removed.
            // The result is an sql conflict.
            for(int id : activeTransactions.values()){
                addStopTime(id ,QDateTime::currentDateTime());
            }

            if(tmpValuemapIds.isEmpty() == false)
            {
                vCDebug(VEIN_LOGGER) << "Batched" << tmpValuemapIds.length() << "queries";
            }

            if(m_dPtr->m_logDB.commit() == false) //do not use assert here, asserts are no-ops in release code
            {
                emit sigDatabaseError(QString("Error in database transaction commit: %1").arg(m_dPtr->m_logDB.lastError().text()));
                Q_ASSERT(false);
            }
        }
        else
        {
            emit sigDatabaseError(QString("Error in database transaction: %1").arg(m_dPtr->m_logDB.lastError().text()));
            Q_ASSERT(false);
        }
        m_dPtr->m_batchVector.clear();
    }
}
} // namespace VeinLogger
