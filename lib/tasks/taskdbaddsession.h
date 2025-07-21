#ifndef TASKDBADDSESSION_H
#define TASKDBADDSESSION_H

#include "vl_abstractloggerdb.h"
#include <tasktemplate.h>
#include <vs_abstracteventsystem.h>

class TaskDbAddSession : public TaskTemplate
{
    Q_OBJECT
public:
    TaskDbAddSession(AbstractLoggerDBPtr loggerDb,
                     VeinStorage::AbstractEventSystem *veinStorage,
                     const QString &dbSessionName,
                     std::shared_ptr<int> sessionId);
    void start() override;
private slots:
    void onAddSessionCompleted(int sessionId);
private:
    QString getEntityName(int entityId) const;
    const QStringList getComponentsFilteredForDb(int entityId) const;

    AbstractLoggerDBPtr m_loggerDb;
    VeinStorage::AbstractEventSystem *m_veinStorage;
    const QString m_dbSessionName;
    std::shared_ptr<int> m_sessionId;
};

#endif // TASKDBADDSESSION_H
