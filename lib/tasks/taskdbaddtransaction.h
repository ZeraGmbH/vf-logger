#ifndef TASKDBADDTRANSACTION_H
#define TASKDBADDTRANSACTION_H

#include "vl_abstractloggerdb.h"
#include <tasktemplate.h>

class TaskDbAddTransaction : public TaskTemplate
{
    Q_OBJECT
public:
    static TaskTemplatePtr create(AbstractLoggerDBPtr loggerDb,
                                  const VeinLogger::StartTransactionParam &param,
                                  std::shared_ptr<int> transactionId);
    TaskDbAddTransaction(AbstractLoggerDBPtr loggerDb,
                         const VeinLogger::StartTransactionParam &param,
                         std::shared_ptr<int> transactionId);
    void start() override;
private slots:
    void onAddTransactionFinished(int transactionId);
private:
    AbstractLoggerDBPtr m_loggerDb;
    const VeinLogger::StartTransactionParam m_param;
    std::shared_ptr<int> m_transactionId;
};

#endif // TASKDBADDTRANSACTION_H
