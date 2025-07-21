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
    bool allConditionsOk = true;
    if (m_loggerDb == nullptr){
        qWarning("No database set in TaskDbAddTransaction");
        allConditionsOk = false;
    }
    if (m_param.m_transactionName.isEmpty()) {
        qWarning("Cannot add nameless transaction in TaskDbAddTransaction");
        allConditionsOk = false;
    }
    if (m_param.m_dbSessionName.isEmpty()) {
        qWarning("Cannot add transaction to nameless session in TaskDbAddTransaction");
        allConditionsOk = false;
    }
    if (m_param.m_contentSets.isEmpty()) {
        qWarning("Cannot add transaction without content sets in TaskDbAddTransaction");
        allConditionsOk = false;
    }

    if(allConditionsOk)
        m_loggerDb->startAddTransaction(m_param);
    else
        finishTaskQueued(false);
}

void TaskDbAddTransaction::onAddTransactionFinished(int transactionId)
{
    *m_transactionId = transactionId;
    finishTask(transactionId != -1);
}
