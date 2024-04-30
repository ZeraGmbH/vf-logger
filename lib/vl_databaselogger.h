#ifndef VL_DATALOGGER_H
#define VL_DATALOGGER_H

#include "vflogger_export.h"
#include "vl_abstractloggerdb.h"
#include <ve_eventsystem.h>
#include <ve_storagesystem.h>
#include <vcmp_componentdata.h>

class DataLoggerPrivate;

namespace VeinLogger
{
class VFLOGGER_EXPORT DatabaseLogger : public VeinEvent::EventSystem
{
    Q_OBJECT
public:
    explicit DatabaseLogger(VeinEvent::StorageSystem *veinStorage, VeinLogger::DBFactory factoryFunction,
                            QObject *parent=nullptr, AbstractLoggerDB::STORAGE_MODE storageMode=AbstractLoggerDB::STORAGE_MODE::TEXT);
    virtual ~DatabaseLogger();
    void processEvent(QEvent *event) override;

    bool loggingEnabled() const;
    int entityId() const;
    QString entityName() const;

signals:
    void sigAddLoggedValue(QString sessionName, QVector<int> transactionIds, int entityId, const QString &componentName, QVariant value, QDateTime timestamp);
    void sigAddEntity(int entityId, const QString &entityName);
    void sigAddComponent(const QString &componentName);
    void sigAddSession(const QString &sessionName,QList<QVariantMap> staticData);

    void sigOpenDatabase(const QString &filePath);

    void sigDatabaseError(const QString &errorString);
    void sigDatabaseReady();
    void sigDatabaseUnloaded();
    void sigLoggingStarted();
    void sigLoggingStopped();
    void sigLogSchedulerActivated();
    void sigLogSchedulerDeactivated();
public slots:
    void setLoggingEnabled(bool enabled);
    bool openDatabase(const QString &filePath);
    void closeDatabase();
    void checkDatabaseStillValid();
    QVariant RPC_deleteSession(QVariantMap parameters);
    void updateSessionList(QStringList sessionNames);

private slots:
    void onModmanSessionChange(QVariant newSession);
private:
    QString getEntityName(int entityId) const;
    void initModmanSessionComponent();
    bool checkDBFilePath(const QString &dbFilePath);
    void handleLoggedComponentsSetNotification(VeinComponent::ComponentData *cData);
    void handleLoggedComponentsChange(QVariant newValue);
    QString handleVeinDbSessionNameSet(QString sessionName);
    bool checkConditionsForStartLog();
    bool isLoggedComponent(int entityId, const QString &componentName) const;
    void addLoggerEntry(int entityId, const QString &componentName);
    void clearLoggerEntries();
    QVariantMap readContentSets();
    void prepareLogging();
    void addValueToDb(const QVariant newValue, const int entityId, const QString componentName);
    void writeCurrentStorageToDb();
    QStringList getComponentsFilteredForDb(int entityId);

    DataLoggerPrivate *m_dPtr = nullptr;
    int m_entityId;
    AbstractLoggerDB::STORAGE_MODE m_storageMode;
    VeinEvent::StorageSystem *m_veinStorage;
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
