#ifndef TASKDBUPDATETRANSACTIONSTOPTIME_H
#define TASKDBUPDATETRANSACTIONSTOPTIME_H

#include "vl_abstractloggerdb.h"
#include <tasktemplate.h>
#include <QDateTime>

class TaskDbUpdateTransactionStopTime : public TaskTemplate
{
    Q_OBJECT
public:
    static TaskTemplatePtr create(AbstractLoggerDBPtr loggerDb,
                                  std::shared_ptr<int> transactionId,
                                  const QDateTime &stopDateTime);
    TaskDbUpdateTransactionStopTime(AbstractLoggerDBPtr loggerDb,
                                     std::shared_ptr<int> transactionId,
                                     const QDateTime &stopDateTime);
    void start() override;
private:
    AbstractLoggerDBPtr m_loggerDb;
    std::shared_ptr<int> m_transactionId;
    QDateTime m_stopDateTime;
};

#endif // TASKDBUPDATETRANSACTIONSTOPTIME_H
