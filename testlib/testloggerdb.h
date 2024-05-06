#ifndef TESTLOGGERDB_H
#define TESTLOGGERDB_H

#include "vl_abstractloggerdb.h"
#include "testdbaddsignaller.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>
#include <QMap>
#include <QList>

class TestLoggerDB : public VeinLogger::AbstractLoggerDB
{
    Q_OBJECT
public:
    TestLoggerDB(TestDbAddSignaller* testSignaller);
    virtual ~TestLoggerDB();

    bool requiresOwnThread() override { return false; }

    bool hasEntityId(int entityId) const override;
    bool hasComponentName(const QString &componentName) const override;
    bool hasSessionName(const QString &sessionName) const override;

    void setStorageMode(STORAGE_MODE storageMode) override;

    void initLocalData() override;
    void addComponent(const QString &componentName) override;
    void addEntity(int entityId, QString entityName) override;
    int addTransaction(const QString &transactionName, const QString &sessionName, const QStringList &contentSets, const QString &guiContextName) override;
    bool addStartTime(int transactionId, QDateTime time) override;
    bool addStopTime(int transactionId,  QDateTime time) override;

    QVariant readSessionComponent(const QString &p_session, const QString &p_entity, const QString &p_component) override;
    int addSession(const QString &sessionName, QList<VeinLogger::DatabaseCommandInterface::ComponentInfo> componentsStoredOncePerSession) override;
    bool deleteSession(const QString &session) override;
    void addLoggedValue(const QString &sessionName, QVector<int> transactionIds, VeinLogger::DatabaseCommandInterface::ComponentInfo component) override;

    static const QLatin1String DBNameOpenOk;
    static const QLatin1String DBNameOpenErrorEarly;
    static const QLatin1String DBNameOpenErrorLate;
    bool openDatabase(const QString &dbPath) override;

    void runBatchedExecution() override;

// Test specific additions
    static TestLoggerDB* getInstance();
    QByteArray getJsonDumpedComponentStored();

    void setCustomerDataAlreadyInDbSession(bool inSession); // remove with m_customerDataAlreadyInDbSession ??
    void deleteDbFile();
    void valuesFromNowOnAreRecorded();

private:
    TestDbAddSignaller* m_testSignaller;
    bool m_customerDataAlreadyInDbSession = false;
    int m_valueWriteCount = 0;

    QString m_openDbPath;
    QStringList m_dbSessionNames = QStringList() << "DbTestSession1" << "DbTestSession2";
    STORAGE_MODE m_storageMode = AbstractLoggerDB::STORAGE_MODE::TEXT;

    QMap<int, QString> m_entitiesAdded;
    QSet<QString> m_componentsAdded;

    QJsonArray m_sessionOnceComponentsAdded;
    QJsonArray m_startStopEvents;

    bool m_valuesAreInitial = true;
    struct InitialValue
    {
        QString sessionName;
        QVariant value;
    };
    QMap<int, QMap<QString, InitialValue>> m_initialValues;

    struct LoggedValue
    {
        QString sessionName;
        int entityId;
        QString componentName;
        QVariant value;
        int dataWriteIdCount;
    };
    QList<LoggedValue> m_loggedValues;

    static TestLoggerDB* m_instance;
};

#endif // TESTLOGGERDB_H
