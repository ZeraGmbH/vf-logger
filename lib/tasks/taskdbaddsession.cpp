#include "taskdbaddsession.h"
#include "vs_abstractdatabase.h"
#include "vl_loggedcomponents.h"

TaskTemplatePtr TaskDbAddSession::create(AbstractLoggerDBPtr loggerDb,
                                         VeinStorage::AbstractEventSystem *veinStorage,
                                         const QString &dbSessionName,
                                         std::shared_ptr<int> sessionId)
{
    return std::make_unique<TaskDbAddSession>(loggerDb,
                                              veinStorage,
                                              dbSessionName,
                                              sessionId);
}

TaskDbAddSession::TaskDbAddSession(AbstractLoggerDBPtr loggerDb,
                                   VeinStorage::AbstractEventSystem *veinStorage,
                                   const QString &dbSessionName,
                                   std::shared_ptr<int> sessionId) :
    m_loggerDb(loggerDb),
    m_veinStorage(veinStorage),
    m_dbSessionName(dbSessionName),
    m_sessionId(sessionId)
{
}

void TaskDbAddSession::start()
{
    bool allConditionsOk = true;
    if (m_loggerDb == nullptr){
        qWarning("No database set in TaskDbAddSession");
        allConditionsOk = false;
    }
    if (m_dbSessionName.isEmpty()) {
        qWarning("Cannot add nameless session in TaskDbAddSession");
        allConditionsOk = false;
    }
    if(allConditionsOk) {
        connect(m_loggerDb.get(), &VeinLogger::AbstractLoggerDB::sigAddSessionCompleted,
                this, &TaskDbAddSession::onAddSessionCompleted);
        QMultiHash<int, QString> tmpStaticComps;
        const VeinStorage::AbstractDatabase* storageDb = m_veinStorage->getDb();
        // Add customer data once per session
        if(storageDb->hasEntity(200))
            for(const QString &comp : getComponentsFilteredForDb(200))
                tmpStaticComps.insert(200, comp);
        // Add status module once per session
        if(storageDb->hasEntity(1150))
            for(const QString &comp : getComponentsFilteredForDb(1150))
                tmpStaticComps.insert(1150, comp);
        QList<VeinLogger::ComponentInfo> componentsAddedOncePerSession;
        for(const int tmpEntityId : tmpStaticComps.uniqueKeys()) { //only process once for every entity
            const QList<QString> tmpComponents = tmpStaticComps.values(tmpEntityId);
            for(const QString &tmpComponentName : tmpComponents) {
                VeinLogger::ComponentInfo component = {
                    tmpEntityId,
                    getEntityName(tmpEntityId),
                    tmpComponentName,
                    storageDb->getStoredValue(tmpEntityId, tmpComponentName),
                    QDateTime::currentDateTime()
                };
                componentsAddedOncePerSession.append(component);
            }
        }
        m_loggerDb->startAddSession(m_dbSessionName, componentsAddedOncePerSession);
    }
    else
        finishTaskQueued(false);
}

void TaskDbAddSession::onAddSessionCompleted(int sessionId)
{
    *m_sessionId = sessionId;
    finishTask(sessionId != -1);
}

QString TaskDbAddSession::getEntityName(int entityId) const
{
    return m_veinStorage->getDb()->getStoredValue(entityId, "EntityName").toString();
}

const QStringList TaskDbAddSession::getComponentsFilteredForDb(int entityId) const
{
    QStringList fullList = m_veinStorage->getDb()->getComponentList(entityId);
    return VeinLogger::LoggedComponents::removeNotStoredOnEntitiesStoringAllComponents(fullList);
}
