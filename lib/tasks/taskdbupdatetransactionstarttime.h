#ifndef TASKDBUPDATETRANSACTIONSTARTTIME_H
#define TASKDBUPDATETRANSACTIONSTARTTIME_H

#include "vl_abstractloggerdb.h"
#include <tasktemplate.h>
#include <QDateTime>

class TaskDbUpdateTransactionStartTime : public TaskTemplate
{
    Q_OBJECT
public:
    static TaskTemplatePtr create(AbstractLoggerDBPtr loggerDb,
                                  std::shared_ptr<int> transactionId,
                                  const QDateTime &startDateTime);
    TaskDbUpdateTransactionStartTime(AbstractLoggerDBPtr loggerDb,
                                     std::shared_ptr<int> transactionId,
                                     const QDateTime &startDateTime);
    void start() override;
private:
    AbstractLoggerDBPtr m_loggerDb;
    std::shared_ptr<int> m_transactionId;
    QDateTime m_startDateTime;
};

#endif // TASKDBUPDATETRANSACTIONSTARTTIME_H
