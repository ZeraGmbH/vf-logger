#ifndef DATABASECOMMANDINTERFACE_H
#define DATABASECOMMANDINTERFACE_H

#include "vl_abstractloggerdb.h"

namespace VeinLogger
{
class DatabaseCommandInterface : public QObject
{
    Q_OBJECT
signals:
    void sigOpenDatabase(const QString &filePath);
    void sigAddEntity(int entityId, const QString &entityName);
    void sigAddComponent(const QString &componentName);
    void sigAddLoggedValue(QString sessionName, QVector<int> transactionIds, int entityId, const QString &componentName, QVariant value, QDateTime timestamp);
    void sigAddSession(const QString &sessionName,QList<QVariantMap> staticData);
public:
    void connectDb(AbstractLoggerDB *db);
};
}

#endif // DATABASECOMMANDINTERFACE_H
