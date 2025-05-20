#ifndef VEINLOGGER_ABSTRACTLOGGERDB_H
#define VEINLOGGER_ABSTRACTLOGGERDB_H

#include "databasecommandinterface.h"
#include <QObject>
#include <QVector>
#include <QDateTime>
#include <QVariant>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
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
    void sigDatabaseError(QString errorString);
    void sigDatabaseReady();
    void sigNewSessionList(QStringList p_sessions);
    void sigDeleteSessionCompleted(QUuid callId, bool success, QString errorMsg, QStringList newSessionsList);
    void sigDeleteTransactionCompleted(QUuid callId, bool success, QString errorMsg);
    void sigDisplaySessionInfosCompleted(QUuid callId, bool success, QString errorMsg, QJsonObject infos);
    void sigListAllSessionsCompleted(QUuid callId, bool success, QString errorMsg, QJsonArray sessions);
    void sigDisplayActualValuesCompleted(QUuid callId, bool success, QString errorMsg, QJsonObject values);
public slots:
    virtual void onOpen(const QString &dbPath) = 0;
    virtual int addSession(const QString &dbSessionName, QList<VeinLogger::DatabaseCommandInterface::ComponentInfo> componentsStoredOncePerSession) = 0 ;
    virtual void onDeleteSession(QUuid callId, const QString &sessionName) = 0;
    virtual QVariant readSessionComponent(const QString &dbSessionName, const QString &entityName, const QString &componentName) = 0;
    virtual void onDisplaySessionsInfos(QUuid callId, const QString &sessionName) = 0;
    virtual void onDeleteTransaction(QUuid callId, const QString &transactionName) = 0;
    virtual void onListAllSessions(QUuid callId) = 0;
    virtual void onDisplayActualValues(QUuid callId, const QString &transactionName) = 0;

    virtual void onAddStartTime(int t_transactionId, QDateTime t_time) = 0;
    virtual int addTransaction(const QString &transactionName, const QString &dbSessionName, const QStringList &contentSets, const QString &guiContextName) = 0;
    virtual void addLoggedValue(const QString &dbSessionName, QVector<int> t_transactionIds, VeinLogger::DatabaseCommandInterface::ComponentInfo component) = 0;
    virtual void runBatchedExecution() = 0; // Another implementation detail which must go
private:
    virtual bool addStopTime(int t_transactionId,  QDateTime t_time) = 0;
};

/// @b factory function alias to create database
using DBFactory = std::function<AbstractLoggerDB *()>;
} // namespace VeinLogger

#endif // VEINLOGGER_ABSTRACTLOGGERDB_H
