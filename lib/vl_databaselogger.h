#ifndef VL_DATALOGGER_H
#define VL_DATALOGGER_H

#include "vflogger_export.h"
#include "vl_abstractloggerdb.h"
#include <ve_eventsystem.h>
#include <ve_commandevent.h>
#include <vcmp_componentdata.h>
#include <QDateTime>

class DataLoggerPrivate;

namespace VeinScript
{
class ScriptSystem;
}

namespace VeinLogger
{
class DataSource;
class QmlLogger;

class VFLOGGER_EXPORT DatabaseLogger : public VeinEvent::EventSystem
{
    Q_OBJECT
public:
    explicit DatabaseLogger(DataSource *t_dataSource, VeinLogger::DBFactory t_factoryFunction, QObject *t_parent=nullptr, AbstractLoggerDB::STORAGE_MODE t_storageMode=AbstractLoggerDB::STORAGE_MODE::TEXT);
    ~DatabaseLogger();
    virtual void processEvent(QEvent *t_event) override;

    static void loadScripts(VeinScript::ScriptSystem *scriptSystem);
    // This is start/stop logging!!!
    void addScript(QmlLogger *script);
    void removeScript(QmlLogger *script);

    bool loggingEnabled() const;
    int entityId() const;
    QString entityName() const;

signals:
    void sigAddLoggedValue(QString t_sessionName, QVector<int> t_transactionIds, int t_entityId, const QString &t_componentName, QVariant t_value, QDateTime t_timestamp);
    void sigAddEntity(int t_entityId, const QString &t_entityName);
    void sigAddComponent(const QString &t_componentName);
    void sigAddSession(const QString &t_sessionName,QList<QVariantMap> p_staticData);

    void sigOpenDatabase(const QString &t_filePath);

    void sigDatabaseError(const QString &t_errorString);
    void sigDatabaseReady();
    void sigDatabaseUnloaded();
    void sigLoggingEnabledChanged(bool t_enabled);
    void sigLoggingStarted();
    void sigLoggingStopped();
    void sigLogSchedulerActivated();
    void sigLogSchedulerDeactivated();
public slots:
    virtual void setLoggingEnabled(bool t_enabled);
    virtual bool openDatabase(const QString &t_filePath);
    virtual void closeDatabase();
    virtual void checkDatabaseStillValid();
    QVariant RPC_deleteSession(QVariantMap p_parameters);
    QVariant RPC_readTransaction(QVariantMap p_parameters);
    QVariant RPC_readSessionComponent(QVariantMap p_parameters);
    /**
     * @brief updateSessionList
     * @param p_sessions: list of sessions stored in open database
     *
     * This function updates the Vein Component ExistingSessions to p_sessions
     */
    virtual void updateSessionList(QStringList p_sessions);

private:
    void initEntity();
    void handleLoggedComponentsTransaction(VeinComponent::ComponentData *cData);

    DataLoggerPrivate *m_dPtr=nullptr;
};
}

#endif // VL_DATALOGGER_H
