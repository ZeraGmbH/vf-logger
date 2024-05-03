#ifndef VL_DATALOGGER_H
#define VL_DATALOGGER_H

#include "databasecommandinterface.h"
#include "vflogger_export.h"
#include "vl_abstractloggerdb.h"
#include <ve_eventsystem.h>
#include <ve_storagesystem.h>
#include <vcmp_componentdata.h>
#include <QFileSystemWatcher>

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

    void dbNameToVein(const QString &filePath);

signals:
    void sigDatabaseError(const QString &errorMsg); // for comptibility - make it go

    void sigLoggingStarted();
    void sigLoggingStopped();
    void sigLogSchedulerActivated();
    void sigLogSchedulerDeactivated();
public slots:
    void setLoggingEnabled(bool enabled);
    bool openDatabase(const QString &filePath);
    void closeDatabase();
    QVariant RPC_deleteSession(QVariantMap parameters);
    void updateSessionList(QStringList sessionNames);

private slots:
    void onModmanSessionChange(QVariant newSession);
    void onDbReady();
    void onDbError(QString errorMsg);
    void checkDatabaseStillValid();
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
    VeinEvent::StorageSystem *m_veinStorage;
    VeinEvent::StorageComponentInterfacePtr m_modmanSessionComponent;

    DBFactory m_databaseFactory;
    DatabaseCommandInterface m_dbCmdInterface;
    AbstractLoggerDB::STORAGE_MODE m_storageMode;
    AbstractLoggerDB *m_database = nullptr;
    bool m_dbReady = false;

    QStringList m_contentSets;
    QMultiHash<int, QString> m_loggedValues;
    QString m_transactionName;
    QString m_dbSessionName;
    int m_transactionId;
    QString m_guiContext;

    QFileSystemWatcher m_deleteWatcher;
    bool m_noUninitMessage = false;

};
}

#endif // VL_DATALOGGER_H
