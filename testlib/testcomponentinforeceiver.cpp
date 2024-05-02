#include "testcomponentinforeceiver.h"

QList<VeinLogger::DatabaseCommandInterface::ComponentInfo> TestComponentInfoReceiver::getInfosReceived()
{
    return m_infosReceived;
}

void TestComponentInfoReceiver::onComponentAddReceived(QString sessionName, QVector<int> transactionIds, VeinLogger::DatabaseCommandInterface::ComponentInfo component)
{
    Q_UNUSED(sessionName)
    Q_UNUSED(transactionIds)
    m_infosReceived.append(component);
}

void TestComponentInfoReceiver::onSessionAddReceived(const QString &sessionName, QList<VeinLogger::DatabaseCommandInterface::ComponentInfo> sessionComponents)
{
    Q_UNUSED(sessionName)
    m_infosReceived.append(sessionComponents);
}
