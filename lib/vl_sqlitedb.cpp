#include "vl_sqlitedb.h"
#include <vh_logging.h>
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
        switch(static_cast<QMetaType::Type>(t_value.type())) { //see http://stackoverflow.com/questions/31290606/qmetatypefloat-not-in-qvarianttype
        case QMetaType::Bool:
        case QMetaType::Float:
        case QMetaType::Double:
        case QMetaType::Int:
        case QMetaType::UInt:
        case QMetaType::Long:
        case QMetaType::LongLong:
        case QMetaType::QString:
        case QMetaType::QByteArray: {
            retVal = t_value;
            break;
        }
        case QMetaType::QVariant: {
            //try to store as string
            retVal = t_value.toString();
            break;
        }
        case QMetaType::QVariantMap: {
            QJsonDocument tmpDoc;
            tmpDoc.setObject(QJsonObject::fromVariantMap(t_value.toMap()));
            retVal = QString::fromUtf8(tmpDoc.toJson());
            break;
        }
        default: {
            const int tmpDataType = QMetaType::type(t_value.typeName());

            if(tmpDataType == QMetaType::type("QList<double>")) { //store as double list
                retVal = convertListToString<QList<double> >(t_value);
            }
            else if(tmpDataType == QMetaType::type("QList<int>")) { //store as int list
                retVal = convertListToString<QList<int> >(t_value);
            }
            else if(tmpDataType == QMetaType::type("QStringList") || tmpDataType == QMetaType::type("QList<QString>")) { //store as string
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
        for( const double var : tmpDoubleList ) {
            tmpResult.append(QString::number(var));
        }
        doubleListValue = QString("%1").arg(tmpResult.join(';'));

        return doubleListValue;
    }


    QVariant readSessionComponent(const QString &p_session, const QString &p_entity, const QString &p_component){
        QVariant retVal;
        if(m_logDB.isOpen()){
            m_sessionCustomerQuery.bindValue(":sessionname",p_session);
            m_sessionCustomerQuery.bindValue(":entity",p_entity);
            m_sessionCustomerQuery.bindValue(":component",p_component);
            if (!m_sessionCustomerQuery.exec()){
                QString err=m_sessionCustomerQuery.lastError().text();
                return retVal;
            }

            while(m_sessionCustomerQuery.next()){
                int fieldNo = m_sessionCustomerQuery.record().indexOf("component_value");
                retVal=m_sessionCustomerQuery.value(fieldNo);
            }
        }
        return retVal;
    }

    QHash<QString, int> m_sessionIds;
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
     * @brief m_sessionMappingInsertQuery
     * store sessionId, valuemapId pairs
     *
     * writes static data to database
     */
    QSqlQuery m_sessionMappingInsertQuery;
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
     * @brief m_sessionInsertQuery
     * add session to database
     *
     * The id is needed beacause writing is done in batches
     * and the ids are managed in this class parallel to the database
     */
    QSqlQuery m_sessionInsertQuery;
    /**
     * @brief m_sessionSequenceQuery
     * get highest session id to database
     *
     * The id is needed beacause writing is done in batches
     * and the ids are managed in this class parallel to the database
     */
    QSqlQuery m_sessionSequenceQuery;
    /**
     * @brief m_readTransactionQuery
     */
    QSqlQuery m_readTransactionQuery;


    /**
     * @brief m_sessionCustomerQuery
     *
     * Read session customer data file name
     */
    QSqlQuery m_sessionCustomerQuery;
    QSqlQuery m_displayInfosQuery;

    int m_valueMapQueryCounter = 0;
    QSqlDatabase m_logDB;

    SQLiteDB::STORAGE_MODE m_storageMode=SQLiteDB::STORAGE_MODE::TEXT;

    SQLiteDB *m_qPtr=nullptr;

    friend class SQLiteDB;
};

SQLiteDB::SQLiteDB() :
    m_dPtr(new DBPrivate(this))
{
    m_dPtr->m_logDB = QSqlDatabase::addDatabase("QSQLITE", "VFLogDB"); //default database
}

SQLiteDB::~SQLiteDB()
{
    runBatchedExecution(); //finish the remaining batch of data
    m_dPtr->m_logDB.close();
    delete m_dPtr;
    QSqlDatabase::removeDatabase("VFLogDB");
}

bool SQLiteDB::hasEntityId(int entityId) const
{
    return m_dPtr->m_entityIds.contains(entityId);
}

bool SQLiteDB::hasComponentName(const QString &componentName) const
{
    return m_dPtr->m_componentIds.contains(componentName);
}

bool SQLiteDB::hasSessionName(const QString &sessionName) const
{
    return m_dPtr->m_sessionIds.contains(sessionName);
}

void SQLiteDB::setStorageMode(AbstractLoggerDB::STORAGE_MODE storageMode)
{
    if(m_dPtr->m_storageMode != storageMode) {
        m_dPtr->m_storageMode = storageMode;
    }
}

bool SQLiteDB::isValidDatabase(QString dbPath)
{
    bool retVal = false;
    QFile dbFile(dbPath);

    if(dbFile.exists()) {
        QSqlDatabase tmpDB = QSqlDatabase::addDatabase("QSQLITE", "TempDB");
        tmpDB.setConnectOptions(QLatin1String("QSQLITE_OPEN_READONLY"));

        tmpDB.setDatabaseName(dbPath);
        if(tmpDB.open()) {
            QSqlQuery schemaValidationQuery(tmpDB);
            if(schemaValidationQuery.exec("SELECT name FROM sqlite_master WHERE type = 'table';"))
            {
                QSet<QString> requiredTables {"sessions", "entities", "components", "valuemap", "transactions"};
                QSet<QString> foundTables;
                while(schemaValidationQuery.next()) {
                    foundTables.insert(schemaValidationQuery.value(0).toString());
                }
                schemaValidationQuery.finish();
                requiredTables.subtract(foundTables);
                if(requiredTables.isEmpty()) {
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
    QSqlQuery sessionQuery("SELECT * FROM sessions WHERE session_name NOT LIKE '_DELETED_%';", m_dPtr->m_logDB);
    QSqlQuery transactionQuery("SELECT * FROM transactions WHERE transaction_name NOT LIKE '_DELETED_%';", m_dPtr->m_logDB);

    while (componentQuery.next()) {
        int componentId = componentQuery.value(0).toInt();
        QString componentName = componentQuery.value(1).toString();
        m_dPtr->m_componentIds.insert(componentName, componentId);
    }
    componentQuery.finish();

    while (entityQuery.next()) {
        int entityId = entityQuery.value(0).toInt();
        m_dPtr->m_entityIds.append(entityId);
    }
    entityQuery.finish();

    while (sessionQuery.next()) {
        int sessionId = sessionQuery.value(0).toInt();
        QString sessionName = sessionQuery.value(1).toString();
        m_dPtr->m_sessionIds.insert(sessionName, sessionId);
    }
    sessionQuery.finish();

    while (transactionQuery.next()) {
        int transactionId = transactionQuery.value("id").toInt();
        QString transactionName = transactionQuery.value("transaction_name").toString();
        m_dPtr->m_transactionIds.insert(transactionId, transactionName);
    }
    transactionQuery.finish();
}

void SQLiteDB::addComponent(const QString &componentName)
{
    if(m_dPtr->m_componentIds.contains(componentName) == false) {
        int nextComponentId=0;

        if(m_dPtr->m_componentSequenceQuery.exec() == true) {
            m_dPtr->m_componentSequenceQuery.next();
            nextComponentId = m_dPtr->m_componentSequenceQuery.value(0).toInt()+1;
        }
        else {
            emit sigDatabaseError(QString("SQLiteDB::addComponent m_componentSequenceQuery failed: %1").arg(m_dPtr->m_componentSequenceQuery.lastError().text()));
            return;
        }

        m_dPtr->m_componentInsertQuery.bindValue(":id", nextComponentId);
        m_dPtr->m_componentInsertQuery.bindValue(":component_name", componentName);
        if(m_dPtr->m_componentInsertQuery.exec() == false) {
            emit sigDatabaseError(QString("SQLiteDB::addComponent m_componentQuery failed: %1").arg(m_dPtr->m_componentInsertQuery.lastError().text()));
            return;
        }
        m_dPtr->m_componentSequenceQuery.finish();

        if(nextComponentId > 0) {
            m_dPtr->m_componentIds.insert(componentName, nextComponentId);
        }
        else {
            emit sigDatabaseError(QString("Error in SQLiteDB::addComponent transaction: %1").arg(m_dPtr->m_logDB.lastError().text()));
            return;
        }
    }
}

void SQLiteDB::addEntity(int entityId, QString entityName)
{
    if(m_dPtr->m_entityIds.contains(entityId) == false) {
        m_dPtr->m_entityInsertQuery.bindValue(":id", entityId);
        m_dPtr->m_entityInsertQuery.bindValue(":entity_name", entityName);

        if(m_dPtr->m_entityInsertQuery.exec() == true) {
            m_dPtr->m_entityIds.append(entityId);
        }
        else {
            emit sigDatabaseError(QString("SQLiteDB::addEntity m_entityInsertQuery failed: %1").arg(m_dPtr->m_entityInsertQuery.lastError().text()));
            return;
        }
    }
}

int SQLiteDB::addTransaction(const QString &transactionName, const QString &sessionName, const QStringList &contentSets, const QString &guiContextName)
{
    int retVal = -1;
    int sessionId = 0;

    //check if session exists. If session does not exist add to list.
    if(m_dPtr->m_sessionIds.contains(sessionName)) {
        sessionId=m_dPtr->m_sessionIds.value(sessionName);
    }
    else {
        int newSession = addSession(sessionName, QList<DatabaseCommandInterface::ComponentInfo>());
        Q_ASSERT(newSession >= 0);
        sessionId = newSession;
    }

    int nexttransactionId = 0;
    if(m_dPtr->m_transactionSequenceQuery.exec() == true)    {
        m_dPtr->m_transactionSequenceQuery.next();
        nexttransactionId = m_dPtr->m_transactionSequenceQuery.value(0).toInt()+1;
    }
    else {
        emit sigDatabaseError(QString("SQLiteDB::addTrancaction m_tranactionSequenceQuery failed: %1").arg(m_dPtr->m_transactionSequenceQuery.lastError().text()));
        return retVal;
    }

    m_dPtr->m_transactionInsertQuery.bindValue(":id", nexttransactionId);
    m_dPtr->m_transactionInsertQuery.bindValue(":sessionid", sessionId);
    m_dPtr->m_transactionInsertQuery.bindValue(":transaction_name", transactionName);
    m_dPtr->m_transactionInsertQuery.bindValue(":contentset_names", contentSets.join(QLatin1Char(',')));
    m_dPtr->m_transactionInsertQuery.bindValue(":guicontext_name", guiContextName);

    if(m_dPtr->m_transactionInsertQuery.exec() == false) {
        emit sigDatabaseError(QString("SQLiteDB::addTransaction m_transactionsQuery failed: %1").arg(m_dPtr->m_transactionInsertQuery.lastError().text()));
        return retVal;
    }
    m_dPtr->m_transactionSequenceQuery.finish();

    if(nexttransactionId > 0) {
        retVal = nexttransactionId;
    }
    else {
        emit sigDatabaseError(QString("Error in SQLiteDB::addTransaction transaction: %1").arg(m_dPtr->m_logDB.lastError().text()));
    }
    return retVal;
}

bool SQLiteDB::addStartTime(int transactionId, QDateTime time)
{
    QSqlQuery logtimeQuery(QString("UPDATE transactions SET (start_time) = ('\%0\') WHERE id = \'%1\';").arg(time.toString()).arg(transactionId),m_dPtr->m_logDB);
    if(logtimeQuery.exec()){
        logtimeQuery.finish();
        return true;
    }
    return false;
}

bool SQLiteDB::addStopTime(int transactionId, QDateTime time)
{
    QSqlQuery logtimeQuery(QString("UPDATE transactions SET (stop_time) = ('\%0\') WHERE id = \'%1\';").arg(time.toString()).arg(transactionId),m_dPtr->m_logDB);
    if(logtimeQuery.exec()){
        logtimeQuery.finish();
        return true;
    }
    return false;
}

bool SQLiteDB::deleteSession(const QString &session)
{
    if(!m_dPtr->m_logDB.isOpen())
        return false;

    if(m_dPtr->m_sessionIds.contains(session)){
        /* Deleting session takes ages for a simple db:
         * On a fresh db with fresh session recording 10s ZeraAll causes delete
         * session take 70s!!!
         * So just rename session and maybe once we have nothing to do we can offer a undo RPC :)
         */
        QSqlQuery updateSessionQuery(m_dPtr->m_logDB);
        updateSessionQuery.prepare("Update sessions set session_name=:sessionNameNew WHERE session_name=:sessionNameOld");
        QString strNewName = QString(QStringLiteral("_DELETED_") + session).left(254);
        updateSessionQuery.bindValue(":sessionNameNew", strNewName);
        updateSessionQuery.bindValue(":sessionNameOld", session);
        updateSessionQuery.exec();
        updateSessionQuery.finish();

        m_dPtr->m_sessionIds.remove(session);
        emit sigNewSessionList(QStringList(m_dPtr->m_sessionIds.keys()));
    }
    return true;
}

int SQLiteDB::addSession(const QString &sessionName, QList<DatabaseCommandInterface::ComponentInfo> componentsStoredOncePerSession)
{
    int retVal = -1;
    if(m_dPtr->m_sessionIds.contains(sessionName) == false) {
        int nextsessionId = 0;
        if(m_dPtr->m_sessionSequenceQuery.exec() == true) {
            m_dPtr->m_sessionSequenceQuery.next();
            nextsessionId = m_dPtr->m_sessionSequenceQuery.value(0).toInt()+1;
        }
        else {
            emit sigDatabaseError(QString("SQLiteDB::addSession m_sessionSequenceQuery failed: %1").arg(m_dPtr->m_sessionSequenceQuery.lastError().text()));
            return retVal;
        }

        m_dPtr->m_sessionInsertQuery.bindValue(":id", nextsessionId);
        m_dPtr->m_sessionInsertQuery.bindValue(":session_name", sessionName);
        if(m_dPtr->m_sessionInsertQuery.exec() == false) {
            emit sigDatabaseError(QString("SQLiteDB::addSession m_sessionInsertQuery failed: %1").arg(m_dPtr->m_sessionInsertQuery.lastError().text()));
            return retVal;
        }
        m_dPtr->m_sessionSequenceQuery.finish();

        QVector<SQLBatchData> batchDataVector;
        for(const auto &component: componentsStoredOncePerSession) {
            addEntityComponent(component);

            SQLBatchData batchData;
            batchData.sessionId = nextsessionId;
            batchData.entityId = component.entityId;
            batchData.componentId = m_dPtr->m_componentIds.value(component.componentName);
            batchData.value = component.value;
            batchData.timestamp = component.timestamp;
            batchDataVector.append(batchData);
        }
        writeStaticData(batchDataVector);

        if(nextsessionId > 0) {
            m_dPtr->m_sessionIds.insert(sessionName, nextsessionId);
            retVal = nextsessionId;
            emit sigNewSessionList(QStringList(m_dPtr->m_sessionIds.keys()));
        }
        else {
            emit sigDatabaseError(QString("Error in SQLiteDB::addSession transaction: %1").arg(m_dPtr->m_logDB.lastError().text()));
            return retVal;
        }
    }
    return retVal;
}

void VeinLogger::SQLiteDB::addEntityComponent(const DatabaseCommandInterface::ComponentInfo &component)
{
    if(!hasEntityId(component.entityId))
        addEntity(component.entityId, component.entityName);
    if(!hasComponentName(component.componentName))
        addComponent(component.componentName);
}

void SQLiteDB::addLoggedValue(int sessionId, const QVector<int> &t_transactionIds, const DatabaseCommandInterface::ComponentInfo &component)
{
    addEntityComponent(component);

    const int componentId = m_dPtr->m_componentIds.value(component.componentName, 0);

    VF_ASSERT(m_dPtr->m_logDB.isOpen() == true, "Database is not open");
    //make sure the ids exist
    VF_ASSERT(componentId > 0, QStringC(QString("(VeinLogger) Unknown componentName: %1").arg(component.componentName)));
    VF_ASSERT(m_dPtr->m_sessionIds.key(sessionId).isEmpty() == false , QStringC(QString("(VeinLogger) Unknown sessionId: %1").arg(sessionId)));
    VF_ASSERT(m_dPtr->m_entityIds.contains(component.entityId) == true, QStringC(QString("(VeinLogger) Unknown entityId: %1").arg(component.entityId)));

    SQLBatchData batchData;
    batchData.sessionId = sessionId;
    batchData.transactionIds = t_transactionIds;
    batchData.entityId = component.entityId;
    batchData.componentId = m_dPtr->m_componentIds.value(component.componentName);
    batchData.value = component.value;
    batchData.timestamp = component.timestamp;

    m_dPtr->m_batchVector.append(batchData);
}

void SQLiteDB::addLoggedValue(const QString &sessionName, QVector<int> transactionIds, DatabaseCommandInterface::ComponentInfo component)
{
    int sessionId = 0;
    if(m_dPtr->m_sessionIds.contains(sessionName)) {
        sessionId=m_dPtr->m_sessionIds.value(sessionName);
    }
    else {
        int newSession = addSession(sessionName,QList<DatabaseCommandInterface::ComponentInfo>());
        Q_ASSERT(newSession >= 0);
        sessionId = newSession;
    }
    addLoggedValue(sessionId, transactionIds, component);
}

QVariant SQLiteDB::readSessionComponent(const QString &session, const QString &enity, const QString &component)
{
    return m_dPtr->readSessionComponent(session, enity, component);
}

QJsonObject SQLiteDB::displaySessionsInfos(const QString &sessionName)
{
    m_dPtr->m_displayInfosQuery.bindValue(":sessionname",sessionName);
    m_dPtr->m_displayInfosQuery.exec();

    QJsonObject sessionObject;
    while(m_dPtr->m_displayInfosQuery.next()){
        QString transactionName = m_dPtr->m_displayInfosQuery.value("transaction_name").toString();
        QString startingTime = m_dPtr->m_displayInfosQuery.value("start_time").toString();
        QString contentset = m_dPtr->m_displayInfosQuery.value("contentset_names").toString();
        QString guicontext = m_dPtr->m_displayInfosQuery.value("guicontext_name").toString();

        QJsonObject transactionObject;
        transactionObject.insert("Time", startingTime);
        transactionObject.insert("contentset", contentset);
        transactionObject.insert("guicontext", guicontext);

        sessionObject.insert(transactionName, transactionObject);
    }

    QJsonObject completeJson;
    completeJson.insert(sessionName, sessionObject);
    return completeJson;
}

void SQLiteDB::setValidTransactions()
{
    m_dPtr->m_transactionIds.clear();
    QSqlQuery validTransactionQuery("SELECT * FROM transactions WHERE transaction_name NOT LIKE '_DELETED_%';", m_dPtr->m_logDB);
    while (validTransactionQuery.next()) {
        int transactionId = validTransactionQuery.value("id").toInt();
        QString transactionName = validTransactionQuery.value("transaction_name").toString();
        m_dPtr->m_transactionIds.insert(transactionId, transactionName);
    }
    validTransactionQuery.finish();
}

bool SQLiteDB::deleteTransaction(const QString &transactionName)
{
    setValidTransactions();

    for(const auto key : m_dPtr->m_transactionIds.keys()) {
        if(m_dPtr->m_transactionIds.value(key) == transactionName){
            QSqlQuery updateTransactionQuery(m_dPtr->m_logDB);
            updateTransactionQuery.prepare("Update transactions set transaction_name=:transactionNameNew WHERE transaction_name=:transactionNameOld");
            QString strNewName = QString(QStringLiteral("_DELETED_") + transactionName).left(254);
            updateTransactionQuery.bindValue(":transactionNameNew", strNewName);
            updateTransactionQuery.bindValue(":transactionNameOld", transactionName);
            updateTransactionQuery.exec();
            updateTransactionQuery.finish();

            m_dPtr->m_transactionIds.remove(key);
            return true;
        }
    }
    return false;
}

void SQLiteDB::onOpen(const QString &dbPath)
{
    QFileInfo fInfo(dbPath);
    if(fInfo.absoluteDir().exists()) {
        // if file exists check if writable. If file
        // does not exist it can not be writable.
        // In this case go on and create file.
        if(fInfo.exists() && !fInfo.isWritable()){
            emit sigDatabaseError(QString("Database is read only"));
            return;
        }
        QSqlError dbError;
        if(m_dPtr->m_logDB.isOpen()) {
            m_dPtr->m_logDB.close();
        }

        m_dPtr->m_logDB.setDatabaseName(dbPath);
        if (!m_dPtr->m_logDB.open()) {
            dbError = m_dPtr->m_logDB.lastError();
            m_dPtr->m_logDB = QSqlDatabase();
        }

        if(dbError.type() != QSqlError::NoError)
            emit sigDatabaseError(QString("Database connection failed error: %1").arg(dbError.text()));
        else {
            //the database was not open when these queries were initialized
            m_dPtr->m_valueMapInsertQuery = QSqlQuery(m_dPtr->m_logDB);
            m_dPtr->m_valueMapSequenceQuery = QSqlQuery(m_dPtr->m_logDB);
            m_dPtr->m_transactionMappingInsertQuery = QSqlQuery(m_dPtr->m_logDB);
            m_dPtr->m_componentInsertQuery = QSqlQuery(m_dPtr->m_logDB);
            m_dPtr->m_componentSequenceQuery = QSqlQuery(m_dPtr->m_logDB);
            m_dPtr->m_entityInsertQuery = QSqlQuery(m_dPtr->m_logDB);
            m_dPtr->m_transactionInsertQuery = QSqlQuery(m_dPtr->m_logDB);
            m_dPtr->m_transactionSequenceQuery = QSqlQuery(m_dPtr->m_logDB);
            m_dPtr->m_sessionSequenceQuery = QSqlQuery(m_dPtr->m_logDB);
            m_dPtr->m_sessionInsertQuery = QSqlQuery(m_dPtr->m_logDB);
            m_dPtr->m_readTransactionQuery = QSqlQuery(m_dPtr->m_logDB);
            m_dPtr->m_sessionMappingInsertQuery = QSqlQuery(m_dPtr->m_logDB);
            m_dPtr->m_sessionCustomerQuery = QSqlQuery(m_dPtr->m_logDB);
            m_dPtr->m_displayInfosQuery = QSqlQuery(m_dPtr->m_logDB);
            //setup database if necessary
            QSqlQuery schemaVersionQuery(m_dPtr->m_logDB);

            if(schemaVersionQuery.exec("pragma schema_version;") == true) { //check if the file is valid (empty or a valid database)
                schemaVersionQuery.first();
                if(schemaVersionQuery.value(0) == 0) { //if there is no database schema or if the file does not exist, then this will create the database and initialize the schema
                    m_dPtr->m_queryReader.setFileName("://sqlite/schema_sqlite.sql");
                    qInfo() << "No schema found in db:"<< dbPath << "creating schema from:" << m_dPtr->m_queryReader.fileName();
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
                /* -- The session is a series of values collected over a variable duration connected to customer data
                 * CREATE TABLE sessions (id INTEGER PRIMARY KEY, session_name VARCHAR(255) NOT NULL UNIQUE) WITHOUT ROWID;
                 */
                m_dPtr->m_transactionInsertQuery.prepare("INSERT INTO transactions (id, sessionid, transaction_name, contentset_names, guicontext_name, start_time, stop_time) VALUES (:id, :sessionid, :transaction_name, :contentset_names, :guicontext_name, :start_time, :stop_time);");
                //executed after the transactions was added to get the last used number
                m_dPtr->m_transactionSequenceQuery.prepare("SELECT MAX(id) FROM transactions;");
                m_dPtr->m_transactionMappingInsertQuery.prepare("INSERT INTO transactions_valuemap VALUES (?, ?);"); //transactionId, valuemapid
                m_dPtr->m_sessionMappingInsertQuery.prepare("INSERT INTO sessions_valuemap VALUES (:sessionId, :valuemapId)");
                m_dPtr->m_readTransactionQuery.prepare("SELECT valuemap.value_timestamp,"
                                                       " valuemap.component_value,"
                                                       " valuemap.id,"
                                                       " components.component_name,"
                                                       " entities.entity_name,"
                                                       " transactions.transaction_name,"
                                                       " sessions.session_name"
                                                       " FROM sessions INNER JOIN transactions ON"
                                                       " sessions.id = transactions.sessionid "
                                                       " INNER JOIN transactions_valuemap ON "
                                                       " transactions.id = transactions_valuemap.transactionsid "
                                                       " INNER JOIN valuemap ON "
                                                       " transactions_valuemap.valueid = valuemap.id "
                                                       " INNER JOIN components ON "
                                                       " valuemap.componentid = components.id "
                                                       " INNER JOIN entities ON valuemap.entityiesid = entities.id where transactions.transaction_name = :transaction AND sessions.session_name = :sessionname ;");

                m_dPtr->m_sessionInsertQuery.prepare("INSERT INTO sessions (id, session_name) VALUES (:id, :session_name);");
                //ecexute after session was added to  get last used number
                m_dPtr->m_sessionSequenceQuery.prepare("SELECT MAX(id) FROM sessions");


                m_dPtr->m_sessionCustomerQuery.prepare("SELECT sessions.session_name, components.component_name,entities.entity_name, valuemap.component_value"
                                                       " FROM sessions INNER JOIN"
                                                       " sessions_valuemap ON sessions.id = sessions_valuemap.sessionsid INNER JOIN"
                                                       " valuemap ON sessions_valuemap.valueid = valuemap.id INNER JOIN entities ON valuemap.entityiesid = entities.id INNER JOIN"
                                                       " components ON valuemap.componentid = components.id"
                                                       " WHERE session_name= :sessionname AND entity_name= :entity AND component_name= :component;");

                m_dPtr->m_displayInfosQuery.prepare("SELECT sessions.session_name,"
                                                    " transactions.transaction_name, transactions.contentset_names, transactions.guicontext_name, transactions.start_time"
                                                    " FROM sessions INNER JOIN"
                                                    " transactions ON sessions.id = transactions.sessionid"
                                                    " WHERE session_name= :sessionname AND transactions.transaction_name NOT LIKE '_DELETED_%';");


                //get next valuemap_id
                if(m_dPtr->m_valueMapSequenceQuery.exec() == false) {
                    emit sigDatabaseError(QString("Error executing m_valueMappingSequenceQuery: %1").arg(m_dPtr->m_valueMapSequenceQuery.lastError().text()));
                    return;
                }
                m_dPtr->m_valueMapSequenceQuery.next();
                m_dPtr->m_valueMapQueryCounter = m_dPtr->m_valueMapSequenceQuery.value(0).toInt()+1;
                //close the query as we read all data from it and it has to be closed to commit the transaction
                m_dPtr->m_valueMapSequenceQuery.finish();

                initLocalData();
                emit sigNewSessionList(QStringList(m_dPtr->m_sessionIds.keys()));

                QSqlQuery tuningQuery(m_dPtr->m_logDB);
                tuningQuery.exec("pragma journal_mode = memory;"); //prevent .journal files and speed up the logging

                emit sigDatabaseReady();
            }
            else { //file is not a database so we don't want to touch it
                emit sigDatabaseError(QString("Unable to open database: %1\nError: %2").arg(dbPath).arg(schemaVersionQuery.lastError().text()));
            }
        }
    }
    else
        emit sigDatabaseError(QString("Error accessing database in directory: %1\nError: directory does not exist").arg(fInfo.absoluteDir().absolutePath()));
}

bool SQLiteDB::isDbStillWitable(const QString &dbPath)
{
    QFileInfo fileInfo(dbPath);
    bool storageOK = true;
    if(!fileInfo.exists()) {
        storageOK = false;
        emit sigDatabaseError(QString("SQLite database file %1 is gone!").arg(dbPath));
    }
    else {
        QStorageInfo storageInfo(fileInfo.absoluteDir());
        if(storageInfo.isValid()) {
            qint64 promilleFree = (storageInfo.bytesAvailable() * 1000) / storageInfo.bytesTotal();
            storageOK = promilleFree > 25 /* 2.5% */;
            if(!storageOK) {
                emit sigDatabaseError("Error volume is almost full");
            }
        }
    }
    return storageOK;
}

void SQLiteDB::runBatchedExecution()
{
    QString dbFileName = m_dPtr->m_logDB.databaseName();
    if(!isDbStillWitable(dbFileName)) {
        return;
    }

    if(m_dPtr->m_logDB.isOpen()) {
        //addBindValue requires QList<QVariant>
        QList<QVariant> tmpTimestamps;
        QList<QVariant> tmpComponentIds;
        QList<QVariant> tmpValuemapIds;
        QList<QVariant> tmpEntityIds;

        //do not use QHash here, the sorted key lists are required
        QMultiMap<QVariant, QVariant> tmpTransactionIds; //session_id, valuemap_id
        QSet<int> activeTransactions;
        QMap<int, QVariant> tmpValues; //valuemap_id, component_value

        //code that is used in both loops
        const auto commonCode = [&](const SQLBatchData &entry) {
            tmpValuemapIds.append(m_dPtr->m_valueMapQueryCounter);
            tmpEntityIds.append(entry.entityId);
            tmpComponentIds.append(entry.componentId);
            //one value can be logged to multiple sessions simultaneously
            for(const int currentTransId : entry.transactionIds) {
                tmpTransactionIds.insert(currentTransId, m_dPtr->m_valueMapQueryCounter);
                activeTransactions.insert(currentTransId);
            }
            tmpTimestamps.append(entry.timestamp);
            ++m_dPtr->m_valueMapQueryCounter;
        };

        if(m_dPtr->m_storageMode == SQLiteDB::STORAGE_MODE::TEXT) {
            for(const SQLBatchData &entry : qAsConst(m_dPtr->m_batchVector)) {
                tmpValues.insert(m_dPtr->m_valueMapQueryCounter, m_dPtr->getTextRepresentation(entry.value)); //store as text
                commonCode(entry);
            }
        }
        else if(m_dPtr->m_storageMode == SQLiteDB::STORAGE_MODE::BINARY) {
            for(const SQLBatchData &entry : qAsConst(m_dPtr->m_batchVector)) {
                tmpValues.insert(m_dPtr->m_valueMapQueryCounter, m_dPtr->getBinaryRepresentation(entry.value)); //store as binary
                commonCode(entry);
            }
        }

        if(m_dPtr->m_logDB.transaction() == true) {
            //valuemap_id, transactionid, value_timestamp, value, entity_id, component_id,
            m_dPtr->m_valueMapInsertQuery.addBindValue(tmpValuemapIds);
            m_dPtr->m_valueMapInsertQuery.addBindValue(tmpTimestamps);
            m_dPtr->m_valueMapInsertQuery.addBindValue(tmpValues.values());
            m_dPtr->m_valueMapInsertQuery.addBindValue(tmpComponentIds);
            m_dPtr->m_valueMapInsertQuery.addBindValue(tmpEntityIds);

            if(m_dPtr->m_valueMapInsertQuery.execBatch() == false) {
                emit sigDatabaseError(QString("Error executing m_valueMapInsertQuery: %1").arg(m_dPtr->m_valueMapInsertQuery.lastError().text()));
                return;
            }
            //transaction_id, valuemap_id
            m_dPtr->m_transactionMappingInsertQuery.addBindValue(tmpTransactionIds.keys());
            m_dPtr->m_transactionMappingInsertQuery.addBindValue(tmpTransactionIds.values());
            if(m_dPtr->m_transactionMappingInsertQuery.execBatch() == false) {
                emit sigDatabaseError(QString("Error executing m_transactionMappingInsertQuery: %1").arg(m_dPtr->m_transactionMappingInsertQuery.lastError().text()));
                return;
            }

            // Add stop time to active transactions. we have to that here becaus a bathc might be written after the script is removed.
            // The result is an sql conflict.
            for(int id : activeTransactions.values()) {
                addStopTime(id ,QDateTime::currentDateTime());
            }

            if(m_dPtr->m_logDB.commit() == false) { //do not use assert here, asserts are no-ops in release code
                emit sigDatabaseError(QString("Error in database transaction commit: %1").arg(m_dPtr->m_logDB.lastError().text()));
                return;
            }
        }
        else {
            emit sigDatabaseError(QString("Error in database transaction: %1").arg(m_dPtr->m_logDB.lastError().text()));
            return;
        }
        m_dPtr->m_batchVector.clear();
    }
}

void SQLiteDB::writeStaticData(QVector<SQLBatchData> p_batchData)
{
    if(m_dPtr->m_logDB.isOpen()) {
        if(!isDbStillWitable(m_dPtr->m_logDB.databaseName())) {
            return;
        }
        //addBindValue requires QList<QVariant>
        QList<QVariant> tmpTimestamps;
        QList<QVariant> tmpComponentIds;
        QList<QVariant> tmpValuemapIds;
        QList<QVariant> tmpEntityIds;

        //do not use QHash here, the sorted key lists are required
        QMultiMap<QVariant, QVariant> tmpSessionIds; //session_id, valuemap_id
        QSet<int> activeTransactions;
        QMap<int, QVariant> tmpValues; //valuemap_id, component_value

        //code that is used in both loops
        const auto commonCode = [&](const SQLBatchData &entry) {
            tmpValuemapIds.append(m_dPtr->m_valueMapQueryCounter);
            tmpEntityIds.append(entry.entityId);
            tmpComponentIds.append(entry.componentId);
            tmpSessionIds.insert(entry.sessionId,m_dPtr->m_valueMapQueryCounter);
            tmpTimestamps.append(entry.timestamp);
            ++m_dPtr->m_valueMapQueryCounter;
        };

        if(m_dPtr->m_storageMode == SQLiteDB::STORAGE_MODE::TEXT) {
            for(const SQLBatchData &entry : qAsConst(p_batchData)) {
                tmpValues.insert(m_dPtr->m_valueMapQueryCounter, m_dPtr->getTextRepresentation(entry.value)); //store as text
                commonCode(entry);
            }
        }
        else if(m_dPtr->m_storageMode == SQLiteDB::STORAGE_MODE::BINARY) {
            for(const SQLBatchData &entry : qAsConst(p_batchData)) {
                tmpValues.insert(m_dPtr->m_valueMapQueryCounter, m_dPtr->getBinaryRepresentation(entry.value)); //store as binary
                commonCode(entry);
            }
        }

        if(m_dPtr->m_logDB.transaction() == true) {
            //valuemap_id, transactionid, value_timestamp, value, entity_id, component_id,
            m_dPtr->m_valueMapInsertQuery.addBindValue(tmpValuemapIds);
            m_dPtr->m_valueMapInsertQuery.addBindValue(tmpTimestamps);
            m_dPtr->m_valueMapInsertQuery.addBindValue(tmpValues.values());
            m_dPtr->m_valueMapInsertQuery.addBindValue(tmpComponentIds);
            m_dPtr->m_valueMapInsertQuery.addBindValue(tmpEntityIds);

            if(m_dPtr->m_valueMapInsertQuery.execBatch() == false) {
                emit sigDatabaseError(QString("Error executing m_valueMapInsertQuery: %1").arg(m_dPtr->m_valueMapInsertQuery.lastError().text()));
                return;
            }
            m_dPtr->m_valueMapInsertQuery.finish();

            //transaction_id, valuemap_id
            m_dPtr->m_sessionMappingInsertQuery.addBindValue(tmpSessionIds.keys());
            m_dPtr->m_sessionMappingInsertQuery.addBindValue(tmpSessionIds.values());
            if(m_dPtr->m_sessionMappingInsertQuery.execBatch() == false) {
                emit sigDatabaseError(QString("Error executing m_transactionMappingInsertQuery: %1").arg(m_dPtr->m_transactionMappingInsertQuery.lastError().text()));
                return;
            }

            m_dPtr->m_sessionMappingInsertQuery.finish();

            // Add stop time to active transactions. we have to that here becaus a bathc might be written after the script is removed.
            // The result is an sql conflict.
            for(int id : activeTransactions.values()) {
                addStopTime(id ,QDateTime::currentDateTime());
            }

            if(m_dPtr->m_logDB.commit() == false) { //do not use assert here, asserts are no-ops in release code
                emit sigDatabaseError(QString("Error in database transaction commit: %1").arg(m_dPtr->m_logDB.lastError().text()));
                return;
            }
        }
        else {
            emit sigDatabaseError(QString("Error in database transaction: %1").arg(m_dPtr->m_logDB.lastError().text()));
            return;
        }
        m_dPtr->m_batchVector.clear();
    }
}
} // namespace VeinLogger
