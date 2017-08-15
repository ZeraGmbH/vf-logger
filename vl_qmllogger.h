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
    Q_PROPERTY(bool loggingEnabled READ loggingEnabled NOTIFY loggingEnabledChanged)
    Q_PROPERTY(bool initializeValues READ initializeValues WRITE setInitializeValues NOTIFY initializeValuesChanged)
  public:
    explicit QmlLogger(QQuickItem *t_parent = 0);
    QString recordName() const;
    bool loggingEnabled() const;
    bool initializeValues() const;
    bool hasLoggerEntry(int t_entityId, const QString &t_componentName) const;

    static void setStaticLogger(DatabaseLogger *t_dbLogger);

    QMultiHash<int, QString> getLoggedValues() const;

  public slots:
    void startLogging();
    void stopLogging();
    void addLoggerEntry(int t_entityId, const QString &t_componentName);
    void removeLoggerEntry(int t_entityId, const QString &t_componentName);
    void setRecordName(QString t_recordName);
    void setInitializeValues(bool initializeValues);

  signals:
    void recordNameChanged(QString t_recordName);
    void loggingEnabledChanged(bool t_loggingEnabled);
    void initializeValuesChanged(bool initializeValues);

  private:
    static DatabaseLogger *s_dbLogger;
    QString m_recordName;
    QMultiHash<int, QString> m_loggedValues;
    bool m_initializeValues=false;
  };
} // namespace VeinLogger

#endif // VEINLOGGER_QMLLOGGER_H
