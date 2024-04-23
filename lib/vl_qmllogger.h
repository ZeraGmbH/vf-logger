#ifndef VEINLOGGER_QMLLOGGER_H
#define VEINLOGGER_QMLLOGGER_H

#include "vflogger_export.h"
#include <QtQuick/QQuickItem>

namespace VeinLogger
{
class DatabaseLogger;

class VFLOGGER_EXPORT QmlLogger : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(bool loggingEnabled READ loggingEnabled NOTIFY loggingEnabledChanged)
    Q_PROPERTY(bool initializeValues READ initializeValues WRITE setInitializeValues NOTIFY initializeValuesChanged)
public:
    explicit QmlLogger(QQuickItem *t_parent = nullptr);
    bool loggingEnabled() const;
    bool initializeValues() const;

    static void setStaticLogger(DatabaseLogger *t_dbLogger);

public slots:
    void startLogging();
    void stopLogging();

    void setInitializeValues(bool t_initializeValues);

signals:
    void loggingEnabledChanged(bool t_loggingEnabled);
    void initializeValuesChanged(bool t_initializeValues);

private:
    static DatabaseLogger *s_dbLogger;
    bool m_initializeValues=false;
};
} // namespace VeinLogger

#endif // VEINLOGGER_QMLLOGGER_H
