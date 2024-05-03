#ifndef VL_DATALOGGER_H
#define VL_DATALOGGER_H

#include "databasecommandinterface.h"
#include "vflogger_export.h"
#include "vl_abstractloggerdb.h"
#include <ve_eventsystem.h>
#include <ve_storagesystem.h>
#include <vcmp_componentdata.h>
#include <QTimer>
#include <QFileSystemWatcher>
#include <QThread>

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
    int entityId() const;
    QString entityName() const;

signals:
    void sigDatabaseError(const QString &errorMsg); // for comptibility - make it go
public slots:
    void setLoggingEnabled(bool enabled);
    void closeDatabase();
    QVariant RPC_deleteSession(QVariantMap parameters);

private slots:
    bool onOpenDatabase(const QString &filePath);
    void onModmanSessionChange(QVariant newSession);
    void onDbReady();
    void onDbError(QString errorMsg);
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
    QLatin1String m_entityName;
    VeinEvent::StorageSystem *m_veinStorage;
    VeinEvent::StorageComponentInterfacePtr m_modmanSessionComponent;

    DBFactory m_databaseFactory;
    DatabaseCommandInterface m_dbCmdInterface;
    AbstractLoggerDB::STORAGE_MODE m_storageMode;
    AbstractLoggerDB *m_database = nullptr;
    QThread m_asyncDatabaseThread;
    QString m_databaseFilePath;
    bool m_dbReady = false;
    bool m_loggingActive = false;
    bool m_scheduledLogging = false;
    QTimer m_countdownUpdateTimer;

    QStringList m_contentSets;
    QMultiHash<int, QString> m_loggedValues;
    QString m_transactionName;
    QString m_dbSessionName;
    int m_transactionId;
    QString m_guiContext;
    QString m_loggerStatusText;

    QFileSystemWatcher m_deleteWatcher;
};
}

#endif // VL_DATALOGGER_H
