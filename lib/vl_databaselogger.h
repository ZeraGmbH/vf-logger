#ifndef VL_DATALOGGER_H
#define VL_DATALOGGER_H

#include "vflogger_export.h"
#include "vl_abstractloggerdb.h"
#include "vl_qmllogger.h"
#include "vl_datasource.h"
#include <ve_commandevent.h>
#include <vsc_scriptsystem.h>
#include <vcmp_componentdata.h>
#include <QDateTime>

class DataLoggerPrivate;

namespace VeinLogger
{
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
    QString getTransactionName() const;
    QString getDbSessionName() const;

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
    void setLoggingEnabled(bool t_enabled);
    bool openDatabase(const QString &t_filePath);
    void closeDatabase();
    void checkDatabaseStillValid();
    QVariant RPC_deleteSession(QVariantMap p_parameters);
    void updateSessionList(QStringList sessionNames);

private slots:
    void onModmanSessionChange(QVariant newSession);
private:
    void initEntity();
    void handleLoggedComponentsTransaction(VeinComponent::ComponentData *cData);
    void handleLoggedComponentsChange(QVariant newValue);
    QVariant handleVeinDbSessionNameSet(QString sessionName);
    bool isLoggedComponent(int entityId, const QString &componentName) const;
    void addLoggerEntry(int t_entityId, const QString &t_componentName);
    void clearLoggerEntries();
    QVariantMap readContentSets();

    QStringList m_contentSets;
    QMultiHash<int, QString> m_loggedValues;
    QString m_transactionName;
    QString m_dbSessionName;
    int m_transactionId;
    QString m_guiContext;
    VeinEvent::StorageComponentInterfacePtr m_modmanSessionComponent;

    DataLoggerPrivate *m_dPtr = nullptr;
};
}

#endif // VL_DATALOGGER_H
