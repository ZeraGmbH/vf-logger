#include "vl_datasource.h"
#include "vl_globallabels.h"
#include <vs_veinhash.h>

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
    DataSource *m_qPtr = nullptr;
    friend class DataSource;
};

class DataSourcePrivateStorage : public DataSourcePrivate
{
    DataSourcePrivateStorage(VeinEvent::StorageSystem *t_dataSource, DataSource *t_public) : DataSourcePrivate(t_public), m_dataSource(t_dataSource)
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

    VeinEvent::StorageSystem *m_dataSource = nullptr;
    friend class DataSource;
};

DataSource::DataSource(VeinEvent::StorageSystem *t_dataSource, QObject *t_parent) :
    QObject(t_parent),
    m_dPtr(new DataSourcePrivateStorage(t_dataSource, this)),
    m_storageSystem(t_dataSource)
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

VeinEvent::StorageSystem *DataSource::getStorageSystem()
{
    return m_storageSystem;
}

} // namespace VeinLogger
