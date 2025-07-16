#include "vl_abstractloggerdb.h"


void VeinLogger::AbstractLoggerDB::startDeleteTransaction(QUuid callId, QString transactionName)
{
    QMetaObject::invokeMethod(this,
                              "onDeleteTransaction",
                              Qt::QueuedConnection,
                              Q_ARG(QUuid, callId),
                              Q_ARG(QString, transactionName));
}

void VeinLogger::AbstractLoggerDB::startDisplaySessionsInfos(QUuid callId, const QString &sessionName)
{
    QMetaObject::invokeMethod(this,
                              "onDisplaySessionsInfos",
                              Qt::QueuedConnection,
                              Q_ARG(QUuid, callId),
                              Q_ARG(QString, sessionName));
}

void VeinLogger::AbstractLoggerDB::startListAllSessions(QUuid callId)
{
    QMetaObject::invokeMethod(this,
                              "onListAllSessions",
                              Qt::QueuedConnection,
                              Q_ARG(QUuid, callId));
}

void VeinLogger::AbstractLoggerDB::startDisplayActualValues(QUuid callId, QString transactionName)
{
    QMetaObject::invokeMethod(this,
                              "onDisplayActualValues",
                              Qt::QueuedConnection,
                              Q_ARG(QUuid, callId),
                              Q_ARG(QString, transactionName));
}

void VeinLogger::AbstractLoggerDB::startDeleteSession(QUuid callId, const QString &sessionName)
{
    QMetaObject::invokeMethod(this,
                              "onDeleteSession",
                              Qt::QueuedConnection,
                              Q_ARG(QUuid, callId),
                              Q_ARG(QString, sessionName));
}

void VeinLogger::AbstractLoggerDB::startFlushToDb()
{
    QMetaObject::invokeMethod(this,
                              "onFlushToDb",
                              Qt::QueuedConnection);
}
