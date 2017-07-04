#ifndef VEINLOGGER_QMLLOGGER_H
#define VEINLOGGER_QMLLOGGER_H

#include "vein-logger_global.h"
#include <QObject>
#include <QMultiHash>

namespace VeinLogger
{
  class DatabaseLogger;
  class VEINLOGGERSHARED_EXPORT QmlLogger : public QObject
  {
    Q_OBJECT
    Q_PROPERTY(QString recordName READ recordName WRITE setRecordName NOTIFY recordNameChanged)
    Q_PROPERTY(bool scriptActive READ scriptActive WRITE setScriptActive NOTIFY scriptActiveChanged)

  public:
    explicit QmlLogger(QObject *t_parent = 0);
    static void setStaticInstance(QmlLogger *t_instance);
    QString recordName() const;
    bool hasLoggerEntry(int t_entityId, const QString &t_componentName) const;

    bool scriptActive() const;

  public slots:
    void registerScriptedLogger();
    void addLoggerEntry(int t_entityId, const QString &t_componentName);
    void removeLoggerEntry(int t_entityId, const QString &t_componentName);
    void setRecordName(QString t_recordName);

    void setScriptActive(bool scriptActive);

  signals:
    void recordNameChanged(QString t_recordName);
    void scriptActiveChanged(bool scriptActive);

  private:
    static DatabaseLogger *s_dbLogger;
    QString m_recordName;
    QMultiHash<int, QString> m_loggedValues;
    bool m_scriptActive;
  };
} // namespace VeinLogger

#endif // VEINLOGGER_QMLLOGGER_H
