#include "taskdbaddtransaction.h"

TaskTemplatePtr TaskDbAddTransaction::create(AbstractLoggerDBPtr loggerDb,
                                             const VeinLogger::StartTransactionParam &param)
{
    return std::make_unique<TaskDbAddTransaction>(loggerDb,
                                                  param);
}

TaskDbAddTransaction::TaskDbAddTransaction(AbstractLoggerDBPtr loggerDb,
                                           const VeinLogger::StartTransactionParam &param) :
    m_loggerDb(loggerDb),
    m_param(param)
{
    connect(m_loggerDb.get(), &VeinLogger::AbstractLoggerDB::sigAddTransactionCompleted,
            this, &TaskDbAddTransaction::finishTask);
}

void TaskDbAddTransaction::start()
{
    m_loggerDb->startAddTransaction(m_param);
}
