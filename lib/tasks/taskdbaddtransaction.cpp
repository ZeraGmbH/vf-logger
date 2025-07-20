#include "taskdbaddtransaction.h"

TaskTemplatePtr TaskDbAddTransaction::create(AbstractLoggerDBPtr loggerDb,
                                             const VeinLogger::StartTransactionParam &param,
                                             std::shared_ptr<int> transactionId)
{
    return std::make_unique<TaskDbAddTransaction>(loggerDb,
                                                  param,
                                                  transactionId);
}

TaskDbAddTransaction::TaskDbAddTransaction(AbstractLoggerDBPtr loggerDb,
                                           const VeinLogger::StartTransactionParam &param,
                                           std::shared_ptr<int> transactionId) :
    m_loggerDb(loggerDb),
    m_param(param),
    m_transactionId(transactionId)
{
    connect(m_loggerDb.get(), &VeinLogger::AbstractLoggerDB::sigAddTransactionCompleted,
            this, &TaskDbAddTransaction::onAddTransactionFinished);
}

void TaskDbAddTransaction::start()
{
    m_loggerDb->startAddTransaction(m_param);
}

void TaskDbAddTransaction::onAddTransactionFinished(int transactionId)
{
    *m_transactionId = transactionId;
    finishTask(transactionId != -1);
}
