#ifndef TASKSDBTAKESNAPSHOTSEQUENCE_H
#define TASKSDBTAKESNAPSHOTSEQUENCE_H

#include "vl_abstractloggerdb.h"
#include <taskcontainersequence.h>

class TasksDbTakeSnapshotSequence : public TaskContainerSequence
{
    Q_OBJECT
public:
    static TaskTemplatePtr create(AbstractLoggerDBPtr loggerDb,
                                  const VeinLogger::StartTransactionParam &param);
    TasksDbTakeSnapshotSequence(AbstractLoggerDBPtr loggerDb,
                                const VeinLogger::StartTransactionParam &param);
private:
    std::shared_ptr<int> m_transactionId;
};

#endif // TASKSDBTAKESNAPSHOTSEQUENCE_H
