#ifndef VEINLOGGER_QMLLOGGER_H
#define VEINLOGGER_QMLLOGGER_H

#include "vein-logger_global.h"
#include <QtQuick/QQuickItem>
#include <QMultiHash>

namespace VeinLogger
{
  class DatabaseLogger;
  class VEINLOGGERSHARED_EXPORT QmlLogger : public QQuickItem
  {
    Q_OBJECT
    Q_PROPERTY(QString recordName READ recordName WRITE setRecordName NOTIFY recordNameChanged)
    Q_PROPERTY(bool scriptActive READ scriptActive WRITE setScriptActive NOTIFY scriptActiveChanged)

  public:
    explicit QmlLogger(QQuickItem *t_parent = 0);
    QString recordName() const;
    bool hasLoggerEntry(int t_entityId, const QString &t_componentName) const;
    bool scriptActive() const;

    static void setStaticLogger(DatabaseLogger *t_dbLogger);

  public slots:
    void registerScriptedLogger();
    void addLoggerEntry(int t_entityId, const QString &t_componentName);
    void removeLoggerEntry(int t_entityId, const QString &t_componentName);
    void setRecordName(QString t_recordName);

    void setScriptActive(bool t_scriptActive);

  signals:
    void recordNameChanged(QString t_recordName);
    void scriptActiveChanged(bool t_scriptActive);

  private:
    static DatabaseLogger *s_dbLogger;
    QString m_recordName;
    QMultiHash<int, QString> m_loggedValues;
    bool m_scriptActive;
  };
} // namespace VeinLogger

#endif // VEINLOGGER_QMLLOGGER_H
