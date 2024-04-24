#ifndef VL_DATALOGGER_H
#define VL_DATALOGGER_H

#include "vflogger_export.h"
#include "vl_abstractloggerdb.h"
#include "vl_datasource.h"
#include <ve_commandevent.h>
#include <vcmp_componentdata.h>
#include <QDateTime>

class DataLoggerPrivate;

namespace VeinLogger
{
class VFLOGGER_EXPORT DatabaseLogger : public VeinEvent::EventSystem
{
    Q_OBJECT
public:
    explicit DatabaseLogger(DataSource *dataSource, VeinLogger::DBFactory factoryFunction, QObject *parent=nullptr, AbstractLoggerDB::STORAGE_MODE storageMode=AbstractLoggerDB::STORAGE_MODE::TEXT);
    virtual ~DatabaseLogger();
    virtual void processEvent(QEvent *t_event) override;

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
    void sigLoggingStarted();
    void sigLoggingStopped();
    void sigLogSchedulerActivated();
    void sigLogSchedulerDeactivated();
public slots:
    void setLoggingEnabled(bool enabled);
    bool openDatabase(const QString &t_filePath);
    void closeDatabase();
    void checkDatabaseStillValid();
    QVariant RPC_deleteSession(QVariantMap p_parameters);
    void updateSessionList(QStringList sessionNames);

private slots:
    void onModmanSessionChange(QVariant newSession);
private:
    void initEntity();
    void tryInitModmanSessionComponent();
    void handleLoggedComponentsTransaction(VeinComponent::ComponentData *cData);
    void handleLoggedComponentsChange(QVariant newValue);
    QVariant handleVeinDbSessionNameSet(QString sessionName);
    bool checkConditionsForStartLog();
    bool isLoggedComponent(int entityId, const QString &componentName) const;
    void addLoggerEntry(int t_entityId, const QString &t_componentName);
    void clearLoggerEntries();
    QVariantMap readContentSets();
    void prepareLogging();
    void addValueToDb(const QVariant newValue, const int entityId, const QString componentName);
    void writeCurrentStorareToDb();

    DataLoggerPrivate *m_dPtr = nullptr;
    int m_entityId;
    AbstractLoggerDB::STORAGE_MODE m_storageMode;
    DataSource *m_dataSource;
    AbstractLoggerDB *m_database = nullptr;
    DBFactory m_databaseFactory;

    QStringList m_contentSets;
    QMultiHash<int, QString> m_loggedValues;
    QString m_transactionName;
    QString m_dbSessionName;
    int m_transactionId;
    QString m_guiContext;
    VeinEvent::StorageComponentInterfacePtr m_modmanSessionComponent;
};
}

#endif // VL_DATALOGGER_H
