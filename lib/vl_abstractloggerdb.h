#ifndef VEINLOGGER_ABSTRACTLOGGERDB_H
#define VEINLOGGER_ABSTRACTLOGGERDB_H

#include "databasecommandinterface.h"
#include <QObject>
#include <QVector>
#include <QDateTime>
#include <QVariant>
#include <functional>

namespace VeinLogger
{
class AbstractLoggerDB : public QObject
{
    Q_OBJECT
public:
    enum class STORAGE_MODE : int {
        TEXT = 0,
        BINARY = 1,
    };
    virtual ~AbstractLoggerDB() = default;
    virtual void setStorageMode(STORAGE_MODE t_storageMode) = 0;
    virtual bool requiresOwnThread() = 0;

    virtual bool hasSessionName(const QString &dbSessionName) const = 0;
signals:
    void sigDatabaseError(const QString &t_errorString);
    void sigDatabaseReady();
    void sigNewSessionList(QStringList p_sessions);
public slots:
    virtual bool openDatabase(const QString &t_dbPath) = 0;
    virtual void initLocalData() = 0;

    virtual int addSession(const QString &dbSessionName, QList<VeinLogger::DatabaseCommandInterface::ComponentInfo> componentsStoredOncePerSession) = 0 ;
    virtual bool deleteSession(const QString &sessionName) = 0;
    virtual QVariant readSessionComponent(const QString &dbSessionName, const QString &entityName, const QString &componentName) = 0;

    virtual bool addStartTime(int t_transactionId, QDateTime t_time) = 0;
    virtual bool addStopTime(int t_transactionId,  QDateTime t_time) = 0;
    virtual int addTransaction(const QString &transactionName, const QString &dbSessionName, const QStringList &contentSets, const QString &guiContextName) = 0;

    virtual void addLoggedValue(const QString &dbSessionName, QVector<int> t_transactionIds, VeinLogger::DatabaseCommandInterface::ComponentInfo component) = 0;

    virtual void runBatchedExecution() = 0; // Another implementation detail which must go
};

/// @b factory function alias to create database
using DBFactory = std::function<AbstractLoggerDB *()>;
} // namespace VeinLogger

#endif // VEINLOGGER_ABSTRACTLOGGERDB_H
