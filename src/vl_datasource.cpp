#include "vl_datasource.h"

#include <veinqml.h>
#include <entitycomponentmap.h>
#include <vs_veinhash.h>
#include "vl_globallabels.h"

namespace VeinLogger
{
class DataSourcePrivate
{
protected:
    DataSourcePrivate(DataSource *t_public) : m_qPtr(t_public)
    {

    }
    virtual ~DataSourcePrivate() {}

    virtual bool hasEntity(int t_entityId) const = 0;
    virtual QVariant getValue(int t_entityId, const QString &t_componentName) const =0;
    virtual QString getEntityName(int t_entityId) const =0;
    virtual QStringList getEntityComponents(int t_entityId) const =0;

private:
    DataSource *m_qPtr=nullptr;
    friend class DataSource;
};

class DataSourcePrivateQml : public DataSourcePrivate
{
    DataSourcePrivateQml(VeinApiQml::VeinQml *t_dataSource, DataSource *t_public) : DataSourcePrivate(t_public), m_dataSource(t_dataSource)
    {

    }
    ~DataSourcePrivateQml() {}

    virtual bool hasEntity(int t_entityId) const override
    {
        return m_dataSource->getEntityById(t_entityId);
    }

    QVariant getValue(int t_entityId, const QString &t_componentName) const override
    {
        return m_dataSource->getEntityById(t_entityId)->value(t_componentName);
    }

    QString getEntityName(int t_entityId) const override
    {
        return m_dataSource->getEntityById(t_entityId)->value(QString("EntityName")).toString();
    }

    QStringList getEntityComponents(int t_entityId) const override
    {
        return m_dataSource->getEntityById(t_entityId)->keys();
    }

    VeinApiQml::VeinQml *m_dataSource=nullptr;
    friend class DataSource;
};

class DataSourcePrivateStorage : public DataSourcePrivate
{
    DataSourcePrivateStorage(VeinStorage::VeinHash *t_dataSource, DataSource *t_public) : DataSourcePrivate(t_public), m_dataSource(t_dataSource)
    {

    }
    ~DataSourcePrivateStorage() {}

    virtual bool hasEntity(int t_entityId) const override
    {
        return m_dataSource->hasEntity(t_entityId);
    }

    QVariant getValue(int t_entityId, const QString &t_componentName) const override
    {
        return m_dataSource->getStoredValue(t_entityId, t_componentName);
    }

    QString getEntityName(int t_entityId) const override
    {
        return m_dataSource->getStoredValue(t_entityId, QString("EntityName")).toString();

    }

    QStringList getEntityComponents(int t_entityId) const override
    {
        return m_dataSource->getEntityComponents(t_entityId);
    }

    VeinStorage::VeinHash *m_dataSource=nullptr;
    friend class DataSource;
};

DataSource::DataSource(VeinApiQml::VeinQml *t_dataSource, QObject *t_parent) : QObject(t_parent), m_dPtr(new DataSourcePrivateQml(t_dataSource, this))
{

}

DataSource::DataSource(VeinStorage::VeinHash *t_dataSource, QObject *t_parent) : QObject(t_parent), m_dPtr(new DataSourcePrivateStorage(t_dataSource, this))
{

}

DataSource::~DataSource()
{
    delete m_dPtr;
}

bool DataSource::hasEntity(int t_entityId) const
{
    return m_dPtr->hasEntity(t_entityId);
}

QVariant DataSource::getValue(int t_entityId, const QString &t_componentName) const
{
    return m_dPtr->getValue(t_entityId, t_componentName);
}

QString DataSource::getEntityName(int t_entityId) const
{
    return m_dPtr->getEntityName(t_entityId);
}

QStringList DataSource::getEntityComponentsForStore(int t_entityId)
{
    QStringList retList = m_dPtr->getEntityComponents(t_entityId);
    // remove common internal components - not meant to store
    QStringList componentsNoStore = VLGlobalLabels::noStoreComponents();
    for(auto noStoreLabel : componentsNoStore) {
        retList.removeAll(noStoreLabel);
    }
    return retList;
}

} // namespace VeinLogger
