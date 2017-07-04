#include "vl_qmllogger.h"

#include <vl_databaselogger.h>
#include <QCoreApplication>
#include <QQmlEngine>

namespace VeinLogger
{
  QmlLogger::QmlLogger(QObject *t_parent) : QObject(t_parent)
  {
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

  void QmlLogger::registerScriptedLogger()
  {
    s_dbLogger->addScript(this);
  }

  void QmlLogger::addLoggerEntry(int t_entityId, const QString &t_componentName)
  {
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

  void QmlLogger::setScriptActive(bool scriptActive)
  {
    if (m_scriptActive != scriptActive)
    {
      m_scriptActive = scriptActive;
      emit scriptActiveChanged(scriptActive);
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
