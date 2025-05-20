#ifndef DATABASECOMMANDINTERFACE_H
#define DATABASECOMMANDINTERFACE_H

#include <QObject>
#include <QVariant>
#include <QDateTime>
#include <QUuid>

namespace VeinLogger
{
class AbstractLoggerDB;

class DatabaseCommandInterface : public QObject
{
    Q_OBJECT
public:
    DatabaseCommandInterface();
    void connectDb(AbstractLoggerDB *db);
    bool isDatabaseConnected();
    struct ComponentInfo
    {
        int entityId;
        QString entityName;
        QString componentName;
        QVariant value;
        QDateTime timestamp;
    };
signals: // commands are send by signals for databases running in other thread
    void sigOpenDatabase(const QString &filePath);
    void sigAddTransaction(const QString &transactionName, const QString &dbSessionName, const QStringList &contentSets, const QString &guiContextName);
    void sigAddStartTime(int transactionId, QDateTime time);
    void sigAddLoggedValue(QString sessionName, QVector<int> transactionIds, VeinLogger::DatabaseCommandInterface::ComponentInfo component);
    void sigAddSession(const QString &sessionName, QList<VeinLogger::DatabaseCommandInterface::ComponentInfo> staticData);
    void sigDeleteSession(QUuid callId, QString sessionName);
    void sigDeleteTransaction(QUuid callId, QString transactionName);
    void sigDisplaySessionInfos(QUuid callId, QString sessionName);
    void sigListAllSessions(QUuid callId);
    void sigDisplayActualValues(QUuid callId, QString transactionName);
    void sigFlushToDb();
private:
    static bool m_componentInfoMetaWasRegistered;
    bool m_databaseConnected = false;
};

}

#endif // DATABASECOMMANDINTERFACE_H
