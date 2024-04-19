#ifndef VEINLOGGER_QMLLOGGER_H
#define VEINLOGGER_QMLLOGGER_H

#include "vflogger_export.h"
#include <QtQuick/QQuickItem>
#include <QMultiHash>
#include <QDateTime>

namespace VeinLogger
{
class DatabaseLogger;

class VFLOGGER_EXPORT QmlLogger : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QString sessionName READ sessionName WRITE setSessionName NOTIFY sessionNameChanged)
    Q_PROPERTY(bool loggingEnabled READ loggingEnabled NOTIFY loggingEnabledChanged)
    Q_PROPERTY(bool initializeValues READ initializeValues WRITE setInitializeValues NOTIFY initializeValuesChanged)
    Q_PROPERTY(QString guiContext READ guiContext WRITE setGuiContext NOTIFY guiContextChanged)
    Q_PROPERTY(QString session READ session WRITE setSession NOTIFY sessionChanged)
public:
    explicit QmlLogger(QQuickItem *t_parent = nullptr);
    QString sessionName() const;
    QString guiContext() const;
    QString session() const;
    bool loggingEnabled() const;
    bool initializeValues() const;

    static void setStaticLogger(DatabaseLogger *t_dbLogger);

    Q_INVOKABLE QStringList getAvailableContentSets();

    int getTransactionId() const;
    void setTransactionId(int transactionId);

    QDateTime getStartTime() const;
    void setStartTime(const QDateTime &startTime);

    QDateTime getStopTime() const;
    void setStopTime(const QDateTime &stopTime);

public slots:
    void startLogging();
    void stopLogging();

    void setSessionName(QString t_sessionName);
    void setInitializeValues(bool t_initializeValues);
    void setGuiContext(const QString &t_guiContext);
    void setSession(const QString &t_session);

signals:
    void sessionNameChanged(QString t_sessionName);
    void sessionChanged(QString t_session);
    void guiContextChanged(QString t_guiContext);
    void loggingEnabledChanged(bool t_loggingEnabled);
    void initializeValuesChanged(bool t_initializeValues);

private:
    static DatabaseLogger *s_dbLogger;
    QString m_session;
    QString m_sessionName;
    int m_transactionId;
    QDateTime m_startTime;
    QDateTime m_stopTime;
    QString m_guiContext;
    bool m_initializeValues=false;
};
} // namespace VeinLogger

#endif // VEINLOGGER_QMLLOGGER_H
