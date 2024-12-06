#ifndef VL_DATALOGGER_H
#define VL_DATALOGGER_H

#include "databasecommandinterface.h"
#include "vflogger_export.h"
#include "vl_abstractloggerdb.h"
#include "vl_loggedcomponents.h"
#include <vs_abstracteventsystem.h>
#include <vcmp_componentdata.h>
#include <vf-cpp-rpc.h>
#include <QTimer>
#include <QFileSystemWatcher>
#include <QThread>

namespace VeinLogger
{
class VFLOGGER_EXPORT DatabaseLogger : public VeinEvent::EventSystem
{
    Q_OBJECT
public:
    explicit DatabaseLogger(VeinStorage::AbstractEventSystem *veinStorage,
                            VeinLogger::DBFactory factoryFunction,
                            QObject *parent = nullptr,
                            QList<int> entitiesWithAllComponentsStoredAlways = QList<int>(),
                            AbstractLoggerDB::STORAGE_MODE storageMode=AbstractLoggerDB::STORAGE_MODE::TEXT);
    virtual ~DatabaseLogger();
    void processEvent(QEvent *event) override;
    int entityId() const;
    QString entityName() const;

signals:
    void sigDatabaseError(const QString &errorMsg); // for comptibility - make it go
public slots:
    void setLoggingEnabled(bool enabled);
    void closeDatabase();
    QVariant RPC_deleteSession(QVariantMap parameters);

private slots:
    void initOnce();
    void onOpenDatabase(const QString &filePath);
    void onModmanSessionChange(QVariant newSession);
    void onDbReady();
    void onDbError(QString errorMsg);
    void onSchedulerCountdownToVein();
    void checkDatabaseStillValid();
    void updateSessionList(QStringList sessionNames);
private:
    QString getEntityName(int entityId) const;
    void dbNameToVein(const QString &filePath);
    void statusTextToVein(const QString &status);
    void initModmanSessionComponent();
    bool checkDBFilePath(const QString &dbFilePath);
    void handleLoggedComponentsSetNotification(VeinComponent::ComponentData *cData);
    void handleLoggedComponentsChange(QVariant newValue);
    void handleContentSetsChange(const QVariant oldValue, const QVariant newValue);
    QString handleVeinDbSessionNameSet(QString sessionName);
    bool checkConditionsForStartLog();
    void prepareLogging();
    void addValueToDb(const QVariant newValue, const int entityId, const QString componentName);
    void writeCurrentStorageToDb();
    QStringList getComponentsFilteredForDb(int entityId);
    void terminateCurrentDb();

    int m_entityId;
    QLatin1String m_entityName;
    VeinStorage::AbstractEventSystem *m_veinStorage;
    VeinStorage::AbstractComponentPtr m_modmanSessionComponent;
    bool m_initDone = false;
    QMap<QString, VfCpp::cVeinModuleRpc::Ptr> m_rpcList;

    DBFactory m_databaseFactory;
    DatabaseCommandInterface m_dbCmdInterface;
    AbstractLoggerDB::STORAGE_MODE m_storageMode;
    AbstractLoggerDB *m_database = nullptr;
    QThread m_asyncDatabaseThread;
    QString m_databaseFilePath;
    bool m_dbReady = false;
    bool m_loggingActive = false;
    bool m_scheduledLogging = false;
    QTimer m_batchedExecutionTimer;
    QTimer m_schedulingTimer;
    QTimer m_countdownUpdateTimer;

    QStringList m_contentSets;
    LoggedComponents m_loggedComponents;
    QString m_transactionName;
    QString m_dbSessionName;
    int m_transactionId;
    QString m_guiContext;
    QString m_loggerStatusText;

    QFileSystemWatcher m_deleteWatcher;
};
}

#endif // VL_DATALOGGER_H
