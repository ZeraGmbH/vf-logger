#include "taskdbupdatetransactionstoptime.h"

TaskTemplatePtr TaskDbUpdateTransactionStopTime::create(AbstractLoggerDBPtr loggerDb,
                                                        std::shared_ptr<int> transactionId,
                                                        const QDateTime &stopDateTime)
{
    return std::make_unique<TaskDbUpdateTransactionStopTime>(loggerDb,
                                                             transactionId,
                                                             stopDateTime);
}

TaskDbUpdateTransactionStopTime::TaskDbUpdateTransactionStopTime(AbstractLoggerDBPtr loggerDb,
                                                                 std::shared_ptr<int> transactionId,
                                                                 const QDateTime &stopDateTime) :
    m_loggerDb(loggerDb),
    m_transactionId(transactionId),
    m_stopDateTime(stopDateTime)
{
    connect(m_loggerDb.get(), &VeinLogger::AbstractLoggerDB::sigUpdateTransactionStopTimeCompleted,
            this, &TaskDbUpdateTransactionStopTime::finishTask);
}

void TaskDbUpdateTransactionStopTime::start()
{
    if (m_loggerDb)
        m_loggerDb->startUpdateTransactionStopTime(*m_transactionId, m_stopDateTime);
    else
        finishTaskQueued(false);
}
