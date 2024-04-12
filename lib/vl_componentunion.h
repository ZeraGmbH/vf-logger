#ifndef COMPONENTUNION_H
#define COMPONENTUNION_H

#include <QMap>
#include <QStringList>

class ComponentUnion
{
public:
    static void uniteComponents(QMap<int, QStringList> &resultMap, int entityID, const QStringList &componentsToSet);
private:
    static bool isAll(const QStringList &components);
};

#endif // COMPONENTUNION_H
