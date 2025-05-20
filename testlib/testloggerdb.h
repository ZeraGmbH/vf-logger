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

    bool hasSessionName(const QString &sessionName) const override;

    void setStorageMode(STORAGE_MODE storageMode) override;

    int addTransaction(const QString &transactionName, const QString &sessionName, const QStringList &contentSets, const QString &guiContextName) override;
    void onAddStartTime(int transactionId, QDateTime time) override;

    QVariant readSessionComponent(const QString &p_session, const QString &p_entity, const QString &p_component) override;
    void onDisplaySessionsInfos(QUuid callId, const QString &sessionName) override;
    void onDeleteTransaction(QUuid callId, const QString &transactionName) override;
    void onListAllSessions(QUuid callId) override;
    void onDisplayActualValues(QUuid callId, const QString &transactionName) override;
    int addSession(const QString &sessionName, QList<VeinLogger::DatabaseCommandInterface::ComponentInfo> componentsStoredOncePerSession) override;
    void onDeleteSession(QUuid callId, const QString &session) override;
    void addLoggedValue(const QString &sessionName, QVector<int> transactionIds, VeinLogger::DatabaseCommandInterface::ComponentInfo component) override;
    void setNextValueWriteCount(int newValueWriteCount);

    void onOpen(const QString &dbPath) override;

    void runBatchedExecution() override;

// Test specific additions
    static TestLoggerDB* getCurrentInstance(); // no singleton!!!
    QByteArray getJsonDumpedComponentStored();

    void deleteDbFile();
    void valuesFromNowOnAreInitial();
    void valuesFromNowOnAreRecorded();

private:
    bool addStopTime(int transactionId,  QDateTime time) override;

    TestDbAddSignaller* m_testSignaller;
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

    struct TransactionInfo {
        QString guiContext;
        QStringList contentSetList;
    };
    typedef QMap<QString, TransactionInfo> Transactions;
    QMap<QString, Transactions> m_sessions;

    static TestLoggerDB* m_instance;
};

#endif // TESTLOGGERDB_H
