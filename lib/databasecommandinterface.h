#ifndef DATABASECOMMANDINTERFACE_H
#define DATABASECOMMANDINTERFACE_H

#include <QObject>
#include <QVariant>
#include <QDateTime>

namespace VeinLogger
{

struct ComponentInfo
{
    int entityId;
    QString entityName;
    QString componentName;
    QVariant value;
    QDateTime timestamp;
};

}

#endif // DATABASECOMMANDINTERFACE_H
