#include "vl_qmllogger.h"

#include <vl_databaselogger.h>
#include <QCoreApplication>
#include <QQmlEngine>

namespace VeinLogger
{
  QmlLogger::QmlLogger(QQuickItem *t_parent) : QQuickItem(t_parent)
  {
    VF_ASSERT(s_dbLogger != nullptr, "Required static logging instance is not set");
    connect(s_dbLogger, SIGNAL(sigLoggingEnabledChanged(bool)), this, SIGNAL(loggingEnabledChanged(bool)));
  }

  QString QmlLogger::recordName() const
  {
    return m_recordName;
  }

  bool QmlLogger::loggingEnabled() const
  {
    return s_dbLogger->loggingEnabled();
  }

  bool QmlLogger::initializeValues() const
  {
    return m_initializeValues;
  }

  bool QmlLogger::hasLoggerEntry(int t_entityId, const QString &t_componentName) const
  {
    return m_loggedValues.contains(t_entityId, t_componentName);
  }

  void QmlLogger::setStaticLogger(DatabaseLogger *t_dbLogger)
  {
    Q_ASSERT(t_dbLogger != nullptr);
    s_dbLogger = t_dbLogger;
  }

  QMultiHash<int, QString> QmlLogger::getLoggedValues() const
  {
    return m_loggedValues;
  }

  void QmlLogger::startLogging()
  {
    s_dbLogger->addScript(this);
  }

  void QmlLogger::stopLogging()
  {
    s_dbLogger->removeScript(this);
  }

  void QmlLogger::addLoggerEntry(int t_entityId, const QString &t_componentName)
  {
    VF_ASSERT(m_recordName.isEmpty() == false, "Logging requires a valid recordName");
    if(m_loggedValues.contains(t_entityId, t_componentName) == false)
    {
      m_loggedValues.insert(t_entityId, t_componentName);
    }
  }

  void QmlLogger::removeLoggerEntry(int t_entityId, const QString &t_componentName)
  {
    if(m_loggedValues.contains(t_entityId, t_componentName))
    {
      m_loggedValues.remove(t_entityId, t_componentName);
    }
  }

  void QmlLogger::setRecordName(QString t_recordName)
  {
    if (m_recordName != t_recordName)
    {
      m_recordName = t_recordName;
      emit recordNameChanged(t_recordName);
    }
  }

  void QmlLogger::setInitializeValues(bool initializeValues)
  {
    if (m_initializeValues == initializeValues)
      return;

    m_initializeValues = initializeValues;
    emit initializeValuesChanged(initializeValues);
  }

  DatabaseLogger *QmlLogger::s_dbLogger = nullptr;
} // namespace VeinLogger

void registerTypes()
{
  // @uri Vein
  using namespace VeinLogger;
  qmlRegisterType<QmlLogger>("VeinLogger", 1, 0, "VeinLogger");
}

Q_COREAPP_STARTUP_FUNCTION(registerTypes)
