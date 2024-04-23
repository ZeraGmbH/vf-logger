#include "vl_qmllogger.h"
#include <vl_databaselogger.h>
#include <QCoreApplication>
#include <QQmlEngine>

namespace VeinLogger
{

DatabaseLogger *QmlLogger::s_dbLogger = nullptr;

QmlLogger::QmlLogger(QQuickItem *t_parent) : QQuickItem(t_parent)
{
    VF_ASSERT(s_dbLogger != nullptr, "Required static logging instance is not set");
    connect(s_dbLogger, &DatabaseLogger::sigLoggingEnabledChanged, this, &QmlLogger::loggingEnabledChanged);
}

bool QmlLogger::loggingEnabled() const
{
    return s_dbLogger->loggingEnabled();
}

bool QmlLogger::initializeValues() const
{
    return m_initializeValues;
}

void QmlLogger::setStaticLogger(DatabaseLogger *t_dbLogger)
{
    Q_ASSERT(t_dbLogger != nullptr);
    s_dbLogger = t_dbLogger;
}

void QmlLogger::startLogging()
{
    bool validConditions = true;
    if(s_dbLogger->getDbSessionName().isEmpty()) {
        validConditions = false;
        qWarning("Logging requires a valid sessionName!");
    }
    if(s_dbLogger->getTransactionName().isEmpty()) {
        validConditions = false;
        qWarning("Logging requires a valid transactionName!");
    }
    if(validConditions)
        s_dbLogger->addScript(this);
}

void QmlLogger::stopLogging()
{
    s_dbLogger->removeScript(this);
}

void QmlLogger::setInitializeValues(bool t_initializeValues)
{
    if (m_initializeValues == t_initializeValues)
        return;
    m_initializeValues = t_initializeValues;
    emit initializeValuesChanged(t_initializeValues);
}

} // namespace VeinLogger

void registerTypes()
{
    // @uri Vein
    using namespace VeinLogger;
    qmlRegisterType<QmlLogger>("VeinLogger", 1, 0, "VeinLogger");
}

Q_COREAPP_STARTUP_FUNCTION(registerTypes)
