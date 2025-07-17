#ifndef VL_DATALOGGER_H
#define VL_DATALOGGER_H

#include "databasecommandinterface.h"
#include "vflogger_export.h"
#include "vl_abstractloggerdb.h"
#include "vl_loggedcomponents.h"
#include "rpcdeletesession.h"
#include "rpcdeletetransaction.h"
#include "rpcdisplaysessionsinfos.h"
#include "rpclistallsessions.h"
#include "rpcdisplayactualvalues.h"
#include <vs_abstracteventsystem.h>
#include <vcmp_componentdata.h>
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

    AbstractLoggerDB *getDb() const;
    bool isDatabaseReady() const;
    bool checkConditionsForStartLog() const;

signals:
    void sigOpenDatabase(const QString &filePath);
    void sigDatabaseError(const QString &errorMsg);
    void sigDeleteSessionCompleted(QUuid callId, bool success, QString errorMsg);
public slots:
    void setLoggingEnabled(bool enabled);
    void closeDatabase();

private slots:
    void initOnce();
    void onModmanSessionChange(QVariant newSession);
    void onDbReady();
    void onDbError(QString errorMsg);
    void onSchedulerCountdownToVein();
    void checkDatabaseStillValid();
    void updateSessionList(QStringList sessionNames);
    void onDeleteSessionCompleted(QUuid callId, bool success, QString errorMsg, QStringList newSessionsList);
    void onListAllSessionsCompleted(QUuid callId, bool success, QString errorMsg, QJsonArray sessions);
private:
    void openDatabase(const QString &filePath);
    QString getEntityName(int entityId) const;
    void dbNameToVein(const QString &filePath);
    void statusTextToVein(const QString &status);
    void initModmanSessionComponent();
    bool checkDBFilePath(const QString &dbFilePath);
    void handleLoggedComponentsSetNotification(VeinComponent::ComponentData *cData);
    void handleLoggedComponentsChange(QVariant newValue);
    void handleContentSetsChange(const QVariant oldValue, const QVariant newValue);
    QString handleVeinDbSessionNameSet(QString sessionName);
    void prepareLogging();
    void addValueToDb(const QVariant newValue, const int entityId, const QString componentName);
    void writeCurrentStorageToDb();
    QStringList getComponentsFilteredForDb(int entityId);
    void terminateCurrentDb();
    void emptyCurrentContentSets();
    void emptyLoggedComponents();

    int m_entityId;
    QLatin1String m_entityName;
    VeinStorage::AbstractEventSystem *m_veinStorage;
    VeinStorage::AbstractComponentPtr m_modmanSessionComponent;
    bool m_initDone = false;
    QMap<QString, VfCpp::VfCppRpcSimplifiedPtr> m_rpcSimplifiedList;
    std::shared_ptr<RpcDeleteSession> m_rpcDeleteSession;
    std::shared_ptr<RpcDeleteTransaction> m_rpcDeleteTransaction;
    std::shared_ptr<RpcDisplaySessionsInfos> m_rpcDisplaySessionsInfos;
    std::shared_ptr<RpcListAllSessions> m_rpcListAllSessions;
    std::shared_ptr<RpcDisplayActualValues> m_rpcDisplayActualValues;

    DBFactory m_databaseFactory;
    std::shared_ptr<DatabaseCommandInterface> m_dbCmdInterface;
    AbstractLoggerDB::STORAGE_MODE m_storageMode;
    AbstractLoggerDB *m_database = nullptr;
    QThread m_asyncDatabaseThread;
    QString m_databaseFilePath;
    QStringList m_existingSessions;
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
