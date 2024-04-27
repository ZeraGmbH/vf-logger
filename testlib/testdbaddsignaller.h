#ifndef TESTDBADDSIGNALLER_H
#define TESTDBADDSIGNALLER_H

#include <QObject>

class TestDbAddSignaller : public QObject
{
    Q_OBJECT
signals:
    void sigEntityAdded(int entityId, QString entityName);
    void sigComponentAdded(QString componentName);
    void sigAddTransaction(const QString &transactionName, const QString &sessionName, const QStringList &contentSets, const QString &guiContextName);
};

#endif // TESTDBADDSIGNALLER_H
