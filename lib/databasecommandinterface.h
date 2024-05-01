#ifndef DATABASECOMMANDINTERFACE_H
#define DATABASECOMMANDINTERFACE_H

#include <QObject>
#include <QVariant>
#include <QDateTime>

namespace VeinLogger
{
class AbstractLoggerDB;

class DatabaseCommandInterface : public QObject
{
    Q_OBJECT
public:
    void connectDb(AbstractLoggerDB *db);
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
    void sigAddEntity(int entityId, const QString &entityName);
    void sigAddComponent(const QString &componentName);
    void sigAddLoggedValue(QString sessionName, QVector<int> transactionIds, VeinLogger::DatabaseCommandInterface::ComponentInfo component);
    void sigAddSession(const QString &sessionName,QList<QVariantMap> staticData);
};
}

#endif // DATABASECOMMANDINTERFACE_H
