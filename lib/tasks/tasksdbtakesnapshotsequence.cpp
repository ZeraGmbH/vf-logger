#include "tasksdbtakesnapshotsequence.h"
#include "taskdbaddtransaction.h"

TaskTemplatePtr TasksDbTakeSnapshotSequence::create(AbstractLoggerDBPtr loggerDb,
                                                    const VeinLogger::StartTransactionParam &param)
{
    return std::make_unique<TasksDbTakeSnapshotSequence>(loggerDb,
                                               param);
}

TasksDbTakeSnapshotSequence::TasksDbTakeSnapshotSequence(AbstractLoggerDBPtr loggerDb,
                                                         const VeinLogger::StartTransactionParam &param) :
    m_transactionId(std::make_shared<int>(-1))
{
    addSub(TaskDbAddTransaction::create(loggerDb, param, m_transactionId));
}
