#ifndef TESTDBADDSIGNALLER_H
#define TESTDBADDSIGNALLER_H

#include <QObject>
#include <QByteArray>
#include <QJsonArray>

class TestDbAddSignaller : public QObject
{
    Q_OBJECT
public:
    TestDbAddSignaller();
    QByteArray dump() const;
signals:
    void sigEntityAdded(int entityId, QString entityName);
    void sigComponentAdded(QString componentName);
    void sigAddTransaction(const QString &transactionName,
                           const QString &sessionName,
                           const QStringList &contentSets,
                           const QString &guiContextName);

private slots:
    void onTransactionAdded(const QString &transactionName,
                            const QString &sessionName,
                            const QStringList &contentSets,
                            const QString &guiContextName);
private:
    QJsonArray m_jsonRecordedSignals;
};

#endif // TESTDBADDSIGNALLER_H
