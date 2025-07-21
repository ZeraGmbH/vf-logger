#include "vl_abstractloggerdb.h"

Q_DECLARE_METATYPE(VeinLogger::ComponentInfo)
Q_DECLARE_METATYPE(VeinLogger::StartTransactionParam)

namespace VeinLogger {

static bool metaTypesRegistered = false;

AbstractLoggerDB::AbstractLoggerDB()
{
    if (!metaTypesRegistered) {
        qRegisterMetaType<QVector<int>>();
        qRegisterMetaType<VeinLogger::ComponentInfo>();
        qRegisterMetaType<QList<VeinLogger::ComponentInfo>>();
        qRegisterMetaType<VeinLogger::StartTransactionParam>();
        metaTypesRegistered = true;
    }
}

void AbstractLoggerDB::startDeleteTransaction(QUuid callId, QString transactionName)
{
    QMetaObject::invokeMethod(this,
                              "onDeleteTransaction",
                              Qt::QueuedConnection,
                              Q_ARG(QUuid, callId),
                              Q_ARG(QString, transactionName));
}

void AbstractLoggerDB::startDisplaySessionsInfos(QUuid callId, const QString &sessionName)
{
    QMetaObject::invokeMethod(this,
                              "onDisplaySessionsInfos",
                              Qt::QueuedConnection,
                              Q_ARG(QUuid, callId),
                              Q_ARG(QString, sessionName));
}

void AbstractLoggerDB::startListAllSessions(QUuid callId)
{
    QMetaObject::invokeMethod(this,
                              "onListAllSessions",
                              Qt::QueuedConnection,
                              Q_ARG(QUuid, callId));
}

void AbstractLoggerDB::startDisplayActualValues(QUuid callId, QString transactionName)
{
    QMetaObject::invokeMethod(this,
                              "onDisplayActualValues",
                              Qt::QueuedConnection,
                              Q_ARG(QUuid, callId),
                              Q_ARG(QString, transactionName));
}

void AbstractLoggerDB::startAddSession(const QString &sessionName,
                                       QList<VeinLogger::ComponentInfo> componentsAddedOncePerSession)
{
    QMetaObject::invokeMethod(this,
                              "startAddSessionQueued",
                              Qt::QueuedConnection,
                              Q_ARG(QString, sessionName),
                              Q_ARG(QList<VeinLogger::ComponentInfo>, componentsAddedOncePerSession));
}

void AbstractLoggerDB::startAddSessionQueued(const QString &sessionName,
                                             QList<ComponentInfo> componentsAddedOncePerSession)
{
    int sessionId = addSession(sessionName, componentsAddedOncePerSession);
    QMetaObject::invokeMethod(this,
                              "sigAddSessionCompleted",
                              Qt::QueuedConnection,
                              Q_ARG(int, sessionId));
}

void AbstractLoggerDB::startDeleteSession(QUuid callId, const QString &sessionName)
{
    QMetaObject::invokeMethod(this,
                              "onDeleteSession",
                              Qt::QueuedConnection,
                              Q_ARG(QUuid, callId),
                              Q_ARG(QString, sessionName));
}

void AbstractLoggerDB::startAddTransaction(const StartTransactionParam &param)
{
    QMetaObject::invokeMethod(this,
                              "startAddTransactionQueued",
                              Qt::QueuedConnection,
                              Q_ARG(VeinLogger::StartTransactionParam, param));
}

void AbstractLoggerDB::startAddTransactionQueued(const StartTransactionParam &param)
{
    int transactionId = addTransaction(param.m_transactionName,
                             param.m_dbSessionName,
                             param.m_contentSets,
                             param.m_guiContextName);
    QMetaObject::invokeMethod(this,
                              "sigAddTransactionCompleted",
                              Qt::QueuedConnection,
                              Q_ARG(int, transactionId));
}

void AbstractLoggerDB::startUpdateTransactionStartTime(int transactionId, const QDateTime &time)
{
    QMetaObject::invokeMethod(this,
                              "startUpdateTransactionStartTimeQueued",
                              Qt::QueuedConnection,
                              Q_ARG(int, transactionId),
                              Q_ARG(QDateTime, time));
}

void AbstractLoggerDB::startUpdateTransactionStartTimeQueued(int transactionId, const QDateTime &time)
{
    bool ok = updateTransactionStartTime(transactionId, time);
    QMetaObject::invokeMethod(this,
                              "sigUpdateTransactionStartTimeCompleted",
                              Qt::QueuedConnection,
                              Q_ARG(bool, ok));
}

void AbstractLoggerDB::startUpdateTransactionStopTimeQueued(int transactionId, const QDateTime &time)
{
    bool ok = updateTransactionStopTime(transactionId, time);
    QMetaObject::invokeMethod(this,
                              "sigUpdateTransactionStopTimeCompleted",
                              Qt::QueuedConnection,
                              Q_ARG(bool, ok));
}

void AbstractLoggerDB::startUpdateTransactionStopTime(int transactionId, const QDateTime &time)
{
    QMetaObject::invokeMethod(this,
                              "startUpdateTransactionStopTimeQueued",
                              Qt::QueuedConnection,
                              Q_ARG(int, transactionId),
                              Q_ARG(QDateTime, time));
}

void AbstractLoggerDB::startFlushToDb()
{
    QMetaObject::invokeMethod(this,
                              "onFlushToDb",
                              Qt::QueuedConnection);
}

}
