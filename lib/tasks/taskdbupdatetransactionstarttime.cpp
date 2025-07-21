#include "taskdbupdatetransactionstarttime.h"

TaskTemplatePtr TaskDbUpdateTransactionStartTime::create(AbstractLoggerDBPtr loggerDb,
                                                         std::shared_ptr<int> transactionId,
                                                         const QDateTime &startDateTime)
{
    return std::make_unique<TaskDbUpdateTransactionStartTime>(loggerDb,
                                                              transactionId,
                                                              startDateTime);
}

TaskDbUpdateTransactionStartTime::TaskDbUpdateTransactionStartTime(AbstractLoggerDBPtr loggerDb,
                                                                   std::shared_ptr<int> transactionId,
                                                                   const QDateTime &startDateTime) :
    m_loggerDb(loggerDb),
    m_transactionId(transactionId),
    m_startDateTime(startDateTime)
{
}

void TaskDbUpdateTransactionStartTime::start()
{
    if (m_loggerDb) {
        connect(m_loggerDb.get(), &VeinLogger::AbstractLoggerDB::sigUpdateTransactionStartTimeCompleted,
                this, &TaskDbUpdateTransactionStartTime::finishTask);
        m_loggerDb->startUpdateTransactionStartTime(*m_transactionId, m_startDateTime);
    }
    else
        finishTaskQueued(false);
}
