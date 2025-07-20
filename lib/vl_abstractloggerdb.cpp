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
    bool ok = addTransaction(param.m_transactionName,
                             param.m_dbSessionName,
                             param.m_contentSets,
                             param.m_guiContextName);
    QMetaObject::invokeMethod(this,
                              "sigAddTransactionCompleted",
                              Qt::QueuedConnection,
                              Q_ARG(bool, ok));
}

void AbstractLoggerDB::startFlushToDb()
{
    QMetaObject::invokeMethod(this,
                              "onFlushToDb",
                              Qt::QueuedConnection);
}

}
