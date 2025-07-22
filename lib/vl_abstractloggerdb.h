#ifndef VEINLOGGER_ABSTRACTLOGGERDB_H
#define VEINLOGGER_ABSTRACTLOGGERDB_H

#include <QObject>
#include <QVector>
#include <QDateTime>
#include <QVariant>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <memory>
#include <functional>

namespace VeinLogger
{

struct ComponentInfo
{
    int entityId;
    QString entityName;
    QString componentName;
    QVariant value;
    QDateTime timestamp;
};

struct StartTransactionParam
{
    QString m_transactionName;
    QString m_dbSessionName;
    QStringList m_contentSets;
    QString m_guiContextName;
};

class AbstractLoggerDB : public QObject
{
    Q_OBJECT
public:
    enum class STORAGE_MODE : int {
        TEXT = 0,
        BINARY = 1,
    };
    AbstractLoggerDB();
    virtual void setStorageMode(STORAGE_MODE t_storageMode) = 0;
    virtual bool requiresOwnThread() = 0;

    // These are not thread safe and should go to private
    virtual bool hasSessionName(const QString &dbSessionName) const = 0;
    virtual int addTransaction(const QString &transactionName,
                               const QString &dbSessionName,
                               const QStringList &contentSets,
                               const QString &guiContextName) = 0;
    virtual bool updateTransactionStartTime(int transactionId, QDateTime t_time) = 0;

    void startDeleteTransaction(QUuid callId, QString transactionName);
    void startDisplaySessionsInfos(QUuid callId, const QString &sessionName);
    void startListAllSessions(QUuid callId);
    void startDisplayActualValues(QUuid callId, QString transactionName);
    void startAddSession(const QString &sessionName, QList<ComponentInfo> componentsAddedOncePerSession);
    void startDeleteSession(QUuid callId, const QString &sessionName);
    void startAddTransaction(const StartTransactionParam &param);
    void startUpdateTransactionStartTime(int transactionId, const QDateTime &time);
    void startUpdateTransactionStopTime(int transactionId, const QDateTime &time);
    void startFlushToDb();

signals:
    void sigDatabaseError(QString errorString);
    void sigDatabaseReady();
    void sigAddSessionCompleted(int sessionId);
    void sigNewSessionList(QStringList p_sessions);
    void sigDeleteSessionCompleted(QUuid callId, bool success, QString errorMsg, QStringList newSessionsList);
    void sigDeleteTransactionCompleted(QUuid callId, bool success, QString errorMsg);
    void sigDisplaySessionInfosCompleted(QUuid callId, bool success, QString errorMsg, QJsonObject infos);
    void sigListAllSessionsCompleted(QUuid callId, bool success, QString errorMsg, QJsonArray sessions);
    void sigDisplayActualValuesCompleted(QUuid callId, bool success, QString errorMsg, QJsonObject values);
    // for tasks
    void sigAddTransactionCompleted(int transactionId);
    void sigUpdateTransactionStartTimeCompleted(bool ok);
    void sigUpdateTransactionStopTimeCompleted(bool ok);
public slots:
    virtual void onOpen(const QString &dbPath) = 0;
    virtual int addSession(const QString &dbSessionName, QList<VeinLogger::ComponentInfo> componentsStoredOncePerSession) = 0;
    virtual void addLoggedValue(const QString &dbSessionName,
                                QVector<int> t_transactionIds,
                                VeinLogger::ComponentInfo component) = 0;
    virtual void onFlushToDb() = 0;

private slots:
    virtual void onDeleteTransaction(QUuid callId, const QString &transactionName) = 0;
    virtual void onDisplaySessionsInfos(QUuid callId, const QString &sessionName) = 0;
    virtual void onListAllSessions(QUuid callId) = 0;
    virtual void onDisplayActualValues(QUuid callId, const QString &transactionName) = 0;
    virtual void onDeleteSession(QUuid callId, const QString &sessionName) = 0;
    void startAddSessionQueued(const QString &sessionName,
                               QList<VeinLogger::ComponentInfo> componentsAddedOncePerSession);
    void startAddTransactionQueued(const VeinLogger::StartTransactionParam &param);
    void startUpdateTransactionStartTimeQueued(int transactionId, const QDateTime &time);
    void startUpdateTransactionStopTimeQueued(int transactionId, const QDateTime &time);

private:
    virtual bool updateTransactionStopTime(int t_transactionId,  QDateTime t_time) = 0;
};

/// @b factory function alias to create database
using DBFactory = std::function<std::shared_ptr<AbstractLoggerDB>()>;
} // namespace VeinLogger

typedef std::shared_ptr<VeinLogger::AbstractLoggerDB> AbstractLoggerDBPtr;

#endif // VEINLOGGER_ABSTRACTLOGGERDB_H
