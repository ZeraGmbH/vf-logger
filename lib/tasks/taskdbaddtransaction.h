#ifndef TASKDBADDTRANSACTION_H
#define TASKDBADDTRANSACTION_H

#include "vl_abstractloggerdb.h"
#include <tasktemplate.h>

class TaskDbAddTransaction : public TaskTemplate
{
    Q_OBJECT
public:
    static TaskTemplatePtr create(AbstractLoggerDBPtr loggerDb,
                                  const VeinLogger::StartTransactionParam &param);
    TaskDbAddTransaction(AbstractLoggerDBPtr loggerDb,
                         const VeinLogger::StartTransactionParam &param);
    void start() override;
private:
    AbstractLoggerDBPtr m_loggerDb;
    const VeinLogger::StartTransactionParam m_param;
};

#endif // TASKDBADDTRANSACTION_H
