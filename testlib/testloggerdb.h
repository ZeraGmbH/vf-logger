#ifndef TESTLOGGERDB_H
#define TESTLOGGERDB_H

#include "vl_abstractloggerdb.h"
#include <QJsonObject>

class TestLoggerDB : public VeinLogger::AbstractLoggerDB
{
    Q_OBJECT
public:
    static TestLoggerDB* getInstance();
    TestLoggerDB();
    virtual ~TestLoggerDB();
    bool hasEntityId(int entityId) const override;
    bool hasComponentName(const QString &componentName) const override;
    bool hasSessionName(const QString &sessionName) const override;

    bool databaseIsOpen() const override;
    QString databasePath() const override;
    void setStorageMode(STORAGE_MODE storageMode) override;
    STORAGE_MODE getStorageMode() const override;
    std::function<bool(QString)> getDatabaseValidationFunction() const override;

    void initLocalData() override;
    void addComponent(const QString &componentName) override;
    void addEntity(int entityId, QString entityName) override;
    int addTransaction(const QString &transactionName, const QString &sessionName, const QString &contentSets, const QString &guiContextName) override;
    bool addStartTime(int transactionId, QDateTime time) override;
    bool addStopTime(int transactionId,  QDateTime time) override;

    QJsonDocument readTransaction(const QString &p_transaction, const QString &p_session) override;
    QVariant readSessionComponent(const QString &p_session, const QString &p_entity, const QString &p_component) override;
    int addSession(const QString &sessionName, QList<QVariantMap> staticData) override;
    bool deleteSession(const QString &session) override;
    void addLoggedValue(const QString &sessionName, QVector<int> transactionIds, int entityId, const QString &componentName, QVariant value, QDateTime timestamp) override;

    static const QLatin1String DBNameOpenOk;
    static const QLatin1String DBNameOpenErrorEarly;
    static const QLatin1String DBNameOpenErrorLate;
    bool openDatabase(const QString &dbPath) override;

    void runBatchedExecution() override;

signals:
    void sigSessionAdded(QJsonObject sessionInfo);
private:
    static TestLoggerDB* m_instance;
    QString m_openDbPath;
    QStringList m_sessionNames = QStringList() << "TestSession1" << "TestSession2";
    STORAGE_MODE m_storageMode = AbstractLoggerDB::STORAGE_MODE::TEXT;
};

#endif // TESTLOGGERDB_H