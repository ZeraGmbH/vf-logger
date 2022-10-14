#include "vl_componentunion.h"

void ComponentUnion::uniteComponents(QMap<int, QStringList> &resultMap, int entityID, const QStringList &componentsToSet)
{
    if(isAll(componentsToSet)) {
        resultMap[entityID] = componentsToSet;
    }
    else if(!resultMap.contains(entityID) || !isAll(resultMap[entityID])) {
        resultMap[entityID].append(componentsToSet);
    }
    resultMap[entityID].removeDuplicates();
}

bool ComponentUnion::isAll(const QStringList &components)
{
    return components.isEmpty();
}
