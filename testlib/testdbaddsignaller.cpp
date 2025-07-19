#include "testdbaddsignaller.h"
#include <testloghelpers.h>
#include <QJsonObject>

TestDbAddSignaller::TestDbAddSignaller()
{
    connect(this, &TestDbAddSignaller::sigAddTransaction,
            this, &TestDbAddSignaller::onTransactionAdded);
}

QByteArray TestDbAddSignaller::dump() const
{
    QJsonObject recordedSignals;
    recordedSignals.insert("RecordedSignals", m_jsonRecordedSignals);
    return TestLogHelpers::dump(recordedSignals);
}

void TestDbAddSignaller::onTransactionAdded(const QString &transactionName,
                                            const QString &sessionName,
                                            const QStringList &contentSets,
                                            const QString &guiContextName)
{
    QJsonObject transactionAdded;
    transactionAdded.insert("transactionName", transactionName);
    transactionAdded.insert("sessionName", sessionName);
    QJsonArray contentSetsJson;
    for (const QString& contentSet : contentSets)
        contentSetsJson.append(contentSet);
    transactionAdded.insert("contentSets", contentSetsJson);
    transactionAdded.insert("guiContextName", guiContextName);

    m_jsonRecordedSignals.append(transactionAdded);
}
