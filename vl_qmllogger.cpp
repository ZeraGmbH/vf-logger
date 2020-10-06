#include "vl_qmllogger.h"

#include <vl_databaselogger.h>
#include <QCoreApplication>
#include <QQmlEngine>
#include "vl_globallabels.h"

namespace VeinLogger
{


QString QmlLogger::m_zeraContentSetPath="";
QString QmlLogger::m_customerContentSetPath="";

QmlLogger::QmlLogger(QQuickItem *t_parent) : QQuickItem(t_parent)
{
    VF_ASSERT(s_dbLogger != nullptr, "Required static logging instance is not set");
    connect(s_dbLogger, SIGNAL(sigLoggingEnabledChanged(bool)), this, SIGNAL(loggingEnabledChanged(bool)));
    VF_ASSERT(!m_zeraContentSetPath.isEmpty(), "zeraContentSetPath is not set");
    VF_ASSERT(!m_customerContentSetPath.isEmpty(), "customerContentSetPath is not set");
    m_contentSetLoader.init(m_zeraContentSetPath,m_customerContentSetPath);

}

QString QmlLogger::recordName() const
{
    return m_recordName;
}

QString QmlLogger::transactionName() const
{
    return m_transactionName;
}

QStringList QmlLogger::contentSets() const
{
    return m_contentSets;
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

void QmlLogger::setContentSetPaths(QString p_zeraPath, QString p_customerPath)
{
    m_zeraContentSetPath=p_zeraPath;
    m_customerContentSetPath=p_customerPath;
}

QMultiHash<int, QString> QmlLogger::getLoggedValues() const
{
    return m_loggedValues;
}

QStringList QmlLogger::readSession()
{
    QStringList result(m_contentSetLoader.contentSetList(m_session).toList());
    return result;
}

QVariantMap QmlLogger::readContentSets()
{
    QVariantMap resultMap;
    for(auto contentSet : m_contentSets) {
        QMap<QString,QVector<QString>> map = m_contentSetLoader.readContentSet(contentSet);
        for(QString key: map.keys()){
            resultMap[key]=QVariant::fromValue(QStringList(map[key].toList()));
        }
    }
    return resultMap;
}

void QmlLogger::startLogging()
{
    if(!m_recordName.isEmpty() && !m_transactionName.isEmpty()){
        VF_ASSERT(m_recordName.isEmpty() == false, "Logging requires a valid recordName");
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
    //VF_ASSERT(m_recordName.isEmpty() == false, "Logging requires a valid recordName");
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

void QmlLogger::setRecordName(QString t_recordName)
{
    if (m_recordName != t_recordName)
    {
        m_recordName = t_recordName;
        emit recordNameChanged(t_recordName);
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

void QmlLogger::setSession(const QString &t_session)
{
    if (m_session == t_session)
        return;

    m_session =  t_session;
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


DatabaseLogger *QmlLogger::s_dbLogger = nullptr;
} // namespace VeinLogger

void registerTypes()
{
    // @uri Vein
    using namespace VeinLogger;
    qmlRegisterType<QmlLogger>("VeinLogger", 1, 0, "VeinLogger");
}

Q_COREAPP_STARTUP_FUNCTION(registerTypes)
