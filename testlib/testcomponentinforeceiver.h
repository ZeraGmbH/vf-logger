#ifndef TESTCOMPONENTINFORECEIVER_H
#define TESTCOMPONENTINFORECEIVER_H

#include "databasecommandinterface.h"
#include <QObject>

class TestComponentInfoReceiver : public QObject
{
    Q_OBJECT
public:
    QList<VeinLogger::DatabaseCommandInterface::ComponentInfo> getInfosReceived();
public slots:
    void onComponentAddReceived(QString sessionName, QVector<int> transactionIds, VeinLogger::DatabaseCommandInterface::ComponentInfo component);
    void onSessionAddReceived(const QString &sessionName, QList<VeinLogger::DatabaseCommandInterface::ComponentInfo> sessionComponents);
private:
    QList<VeinLogger::DatabaseCommandInterface::ComponentInfo> m_infosReceived;
};

#endif // TESTCOMPONENTINFORECEIVER_H
