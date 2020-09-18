#ifndef VEINLOGGER_QMLLOGGER_H
#define VEINLOGGER_QMLLOGGER_H

#include "globalIncludes.h"
#include <QtQuick/QQuickItem>
#include <QMultiHash>
#include <QDateTime>

#include "jsoncontextloader.h"

namespace VeinLogger
{
class DatabaseLogger;
class VFLOGGER_EXPORT QmlLogger : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QString recordName READ recordName WRITE setRecordName NOTIFY recordNameChanged)
    Q_PROPERTY(QString transactionName READ transactionName WRITE setTransactionName NOTIFY transactionNameChanged)
    Q_PROPERTY(bool loggingEnabled READ loggingEnabled NOTIFY loggingEnabledChanged)
    Q_PROPERTY(bool initializeValues READ initializeValues WRITE setInitializeValues NOTIFY initializeValuesChanged)
    Q_PROPERTY(QString context READ context WRITE setContext NOTIFY contextChanged)
    Q_PROPERTY(QString session READ session WRITE setSession NOTIFY sessionChanged)
public:
    explicit QmlLogger(QQuickItem *t_parent = nullptr);
    QString recordName() const;
    QString transactionName() const;
    QString context() const;
    QString session() const;
    bool loggingEnabled() const;
    bool initializeValues() const;
    bool hasLoggerEntry(int t_entityId, const QString &t_componentName) const;

    static void setStaticLogger(DatabaseLogger *t_dbLogger);
    static void setContextPath(QString p_zeraPath,QString p_customerPath);

    QMultiHash<int, QString> getLoggedValues() const;

    Q_INVOKABLE QStringList readSession();
    Q_INVOKABLE QVariantMap readContext();

    int getTransactionId() const;
    void setTransactionId(int transactionId);

    QDateTime getStartTime() const;
    void setStartTime(const QDateTime &startTime);

    QDateTime getStopTime() const;
    void setStopTime(const QDateTime &stopTime);

public slots:
    void startLogging();
    void stopLogging();
    void addLoggerEntry(int t_entityId, const QString &t_componentName);
    void removeLoggerEntry(int t_entityId, const QString &t_componentName);
    void clearLoggerEntries();
    void setRecordName(QString t_recordName);
    void setTransactionName(const QString &t_transactionName);
    void setInitializeValues(bool t_initializeValues);
    void setContext(const QString &t_context);
    void setSession(const QString &t_session);

signals:
    void recordNameChanged(QString t_recordName);
    void transactionNameChanged(QString t_transactionName);
    void contextChanged(QString t_context);
    void sessionChanged(QString t_session);
    void loggingEnabledChanged(bool t_loggingEnabled);
    void initializeValuesChanged(bool t_initializeValues);

private:
    static DatabaseLogger *s_dbLogger;
    static QString m_zeraContextPath;
    static QString m_customerContextPath;
    QString m_session;
    QString m_recordName;
    QString m_transactionName;
    int m_transactionId;
    QDateTime m_startTime;
    QDateTime m_stopTime;
    QString m_context;
    QMultiHash<int, QString> m_loggedValues;
    bool m_initializeValues=false;
    JsonContextLoader m_contextLoader;
};
} // namespace VeinLogger

#endif // VEINLOGGER_QMLLOGGER_H
