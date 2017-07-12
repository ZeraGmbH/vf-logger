#include "vl_qmllogger.h"

#include <vl_databaselogger.h>
#include <QCoreApplication>
#include <QQmlEngine>

namespace VeinLogger
{
  QmlLogger::QmlLogger(QQuickItem *t_parent) : QQuickItem(t_parent)
  {
    VF_ASSERT(s_dbLogger != nullptr, "Required static logging instance is not set");
  }

  QString QmlLogger::recordName() const
  {
    return m_recordName;
  }

  bool QmlLogger::hasLoggerEntry(int t_entityId, const QString &t_componentName) const
  {
    return m_loggedValues.contains(t_entityId, t_componentName);
  }

  bool QmlLogger::scriptActive() const
  {
    return m_scriptActive;
  }

  void QmlLogger::setStaticLogger(DatabaseLogger *t_dbLogger)
  {
    Q_ASSERT(t_dbLogger != nullptr);
    s_dbLogger = t_dbLogger;
  }

  void QmlLogger::registerScriptedLogger()
  {
    s_dbLogger->addScript(this);
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

  void QmlLogger::setScriptActive(bool t_scriptActive)
  {
    if (m_scriptActive != t_scriptActive)
    {
      m_scriptActive = t_scriptActive;
      emit scriptActiveChanged(t_scriptActive);
    }
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
