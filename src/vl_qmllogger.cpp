#include "vl_qmllogger.h"
#include "vl_globallabels.h"
#include "vf_loggercontenthandler.h"
#include <vl_databaselogger.h>
#include <QCoreApplication>
#include <QQmlEngine>

namespace VeinLogger
{

QList<QmlLogger::LoggerConfigEnvironment> QmlLogger::m_loggerConfigEnvironment;
DatabaseLogger *QmlLogger::s_dbLogger = nullptr;

QmlLogger::QmlLogger(QQuickItem *t_parent) : QQuickItem(t_parent)
{
    VF_ASSERT(s_dbLogger != nullptr, "Required static logging instance is not set");
    connect(s_dbLogger, SIGNAL(sigLoggingEnabledChanged(bool)), this, SIGNAL(loggingEnabledChanged(bool)));
    for(auto &env: m_loggerConfigEnvironment) {
        VF_ASSERT(env.m_loggerContentHandler != nullptr, "Required static json content handler instance is not set");
        env.m_loggerContentHandler->setConfigFileDir(env.m_configFileDir);
    }
}

QString QmlLogger::sessionName() const
{
    return m_sessionName;
}

QString QmlLogger::transactionName() const
{
    return m_transactionName;
}

QStringList QmlLogger::contentSets() const
{
    return m_contentSets;
}

QString QmlLogger::guiContext() const
{
    return m_guiContext;
}

QString QmlLogger::session() const
{
    return m_session;
}

bool QmlLogger::loggingEnabled() const
{
    return s_dbLogger->loggingEnabled();
}

bool QmlLogger::initializeValues() const
{
    return m_initializeValues;
}

bool QmlLogger::isLoggedComponent(int t_entityId, const QString &t_componentName) const
{
    bool storeComponent = false;
    if(m_loggedValues.contains(t_entityId, VLGlobalLabels::allComponentsName())) {
        QStringList componentsNoStore = VLGlobalLabels::noStoreComponents();
        storeComponent = !componentsNoStore.contains(t_componentName);
    }
    else {
        storeComponent = m_loggedValues.contains(t_entityId, t_componentName);
    }
    return storeComponent;
}

void QmlLogger::setStaticLogger(DatabaseLogger *t_dbLogger)
{
    Q_ASSERT(t_dbLogger != nullptr);
    s_dbLogger = t_dbLogger;
}

void QmlLogger::setJsonEnvironment(const QString configFileDir, std::shared_ptr<LoggerContentHandler> loggerContentHandler)
{
    m_loggerConfigEnvironment.append({configFileDir, loggerContentHandler});
}

QMultiHash<int, QString> QmlLogger::getLoggedValues() const
{
    return m_loggedValues;
}

QStringList QmlLogger::getAvailableContentSets()
{
    QStringList ret;
    for(auto &confEnv: m_loggerConfigEnvironment) {
        ret.append(confEnv.m_loggerContentHandler->getAvailableContentSets());
    }
    return ret;
}

static void mergeComponents(QMap<int, QStringList> &resultMap, int entityID, const QStringList &componentsToSet)
{
    if(!resultMap.contains(entityID)) {
        resultMap[entityID] = componentsToSet;
    }
    else if(!resultMap[entityID].isEmpty()) {
        if(componentsToSet.isEmpty()) {
            resultMap[entityID] = componentsToSet;
        }
        else {
            QStringList tmpList = resultMap[entityID] + componentsToSet;
            tmpList.removeDuplicates();
            resultMap[entityID] = tmpList;
        }
    }
}

QVariantMap QmlLogger::readContentSets()
{
    typedef QMap<int, QStringList> TEcMap;
    TEcMap tmpResultMap;
    for(auto &contentSet : m_contentSets) {
        for(auto &confEnv: m_loggerConfigEnvironment) {
            TEcMap ecMap = confEnv.m_loggerContentHandler->getEntityComponents(contentSet);
            TEcMap::const_iterator iter;
            for(iter=ecMap.constBegin(); iter!=ecMap.constEnd(); ++iter) {
                mergeComponents(tmpResultMap, iter.key(), iter.value());
            }
        }
    }
    QVariantMap resultMap;
    TEcMap::const_iterator iter;
    for(iter=tmpResultMap.constBegin(); iter!=tmpResultMap.constEnd(); ++iter) {
        resultMap[QString::number(iter.key())] = tmpResultMap[iter.key()];
    }
    return resultMap;
}

void QmlLogger::startLogging()
{
    if(!m_sessionName.isEmpty() && !m_transactionName.isEmpty()){
        VF_ASSERT(m_sessionName.isEmpty() == false, "Logging requires a valid sessionName");
        VF_ASSERT(m_transactionName.isEmpty() == false, "Logging requires a valid transactionName");
        s_dbLogger->addScript(this);
    }
}

void QmlLogger::stopLogging()
{
    s_dbLogger->removeScript(this);
}

void QmlLogger::addLoggerEntry(int t_entityId, const QString &t_componentName)
{
    //VF_ASSERT(m_sessionName.isEmpty() == false, "Logging requires a valid sessionName");
    if(m_loggedValues.contains(t_entityId, t_componentName) == false)
    {
        m_loggedValues.insert(t_entityId, t_componentName);
    }
}

void QmlLogger::removeLoggerEntry(int t_entityId, const QString &t_componentName)
{
    if(m_loggedValues.contains(t_entityId, t_componentName))
    {
        m_loggedValues.remove(t_entityId, t_componentName);
    }
}

void QmlLogger::clearLoggerEntries()
{
    m_loggedValues.clear();
}

void QmlLogger::setSessionName(QString t_sessionName)
{
    if (m_sessionName != t_sessionName)
    {
        m_sessionName = t_sessionName;
        emit sessionNameChanged(t_sessionName);
    }
}

void QmlLogger::setTransactionName(const QString &t_transactionName)
{
    if (m_transactionName != t_transactionName)
    {
        m_transactionName= t_transactionName;
        emit transactionNameChanged(t_transactionName);
    }
}

void QmlLogger::setInitializeValues(bool t_initializeValues)
{
    if (m_initializeValues == t_initializeValues)
        return;

    m_initializeValues = t_initializeValues;
    emit initializeValuesChanged(t_initializeValues);
}

void QmlLogger::setContentSets(const QStringList &t_contentSets)
{
    if (m_contentSets == t_contentSets)
        return;

    m_contentSets =  t_contentSets;
    emit contentSetsChanged(t_contentSets);
}

void QmlLogger::setGuiContext(const QString &t_guiContext)
{
    if(m_guiContext == t_guiContext)
        return;
    m_guiContext = t_guiContext;
    emit guiContextChanged(t_guiContext);
}

void QmlLogger::setSession(const QString &t_session)
{
    if (m_session == t_session)
        return;

    m_session =  t_session;
    for (auto &env : m_loggerConfigEnvironment) {
        env.m_loggerContentHandler->setSession(m_session);
    }
    emit sessionChanged(t_session);
}

QDateTime QmlLogger::getStopTime() const
{
    return m_stopTime;
}

void QmlLogger::setStopTime(const QDateTime &stopTime)
{
    m_stopTime = stopTime;
}

QDateTime QmlLogger::getStartTime() const
{
    return m_startTime;
}

void QmlLogger::setStartTime(const QDateTime &startTime)
{
    m_startTime = startTime;
}

int QmlLogger::getTransactionId() const
{
    return m_transactionId;
}

void QmlLogger::setTransactionId(int transactionId)
{
    m_transactionId = transactionId;
}

} // namespace VeinLogger

void registerTypes()
{
    // @uri Vein
    using namespace VeinLogger;
    qmlRegisterType<QmlLogger>("VeinLogger", 1, 0, "VeinLogger");
}

Q_COREAPP_STARTUP_FUNCTION(registerTypes)
