#include "testinsertspiestojson.h"
#include <testloghelpers.h>
#include <QJsonObject>
#include <QJsonArray>

QByteArray TestInsertSpiesToJson::spiesToJsonAndClear(QSignalSpy &spyDbEntitiesInserted, QSignalSpy &spyDbComponentsInserted)
{
    QJsonArray entitiesInserted = spyEntitiesToJson(spyDbEntitiesInserted);
    QJsonArray componentsInserted = spyComponentsToJson(spyDbComponentsInserted);

    spyDbEntitiesInserted.clear();
    spyDbComponentsInserted.clear();

    QJsonObject all;
    all.insert("EntititiesInserted", entitiesInserted);
    all.insert("ComponentsInserted", componentsInserted);
    return TestLogHelpers::dump(all);
}

QJsonArray TestInsertSpiesToJson::spyEntitiesToJson(const QSignalSpy &spyDbEntitiesInserted)
{
    QMap<int, QJsonObject> entitiesInsertedSorted;
    for(const auto &spyEntityInserted : spyDbEntitiesInserted) {
        int entityId = spyEntityInserted[0].toInt();
        QJsonObject entityInserted;
        entityInserted.insert("EntityId", entityId);
        entityInserted.insert("EntityName", spyEntityInserted[1].toString());
        entitiesInsertedSorted.insert(entityId, entityInserted);
    }
    QJsonArray entitiesInserted;
    for(const auto &entity : entitiesInsertedSorted)
        entitiesInserted.append(entity);
    return entitiesInserted;
}

QJsonArray TestInsertSpiesToJson::spyComponentsToJson(const QSignalSpy &spyDbComponentsInserted)
{
    QMap<QString, QJsonObject> componentsInsertedSorted;
    for(const auto &spyComponentInserted : spyDbComponentsInserted) {
        QString componentName = spyComponentInserted[0].toString();
        QJsonObject componentInserted;
        componentInserted.insert("ComponentName", componentName);
        componentsInsertedSorted.insert(componentName, componentInserted);
    }
    QJsonArray componentsInserted;
    for(const auto &component : componentsInsertedSorted)
        componentsInserted.append(component);
    return componentsInserted;
}
