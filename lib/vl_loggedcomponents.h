#ifndef VL_LOGGEDCOMPONENTS_H
#define VL_LOGGEDCOMPONENTS_H

#include <QString>
#include <QHash>
#include <QSet>

namespace VeinLogger
{

class LoggedComponents
{
public:
    void clear();
    void addComponent(int entityId, const QString &componentName);
    bool isLoggedComponent(int entityId, const QString &componentName) const;
    QList<int> getEntities() const;
    QStringList getComponents(int entityId) const;

private:
    bool contains(int entityId, const QString &componentName) const;
    QHash<int, QSet<QString>> m_components;
};

}
#endif // VL_LOGGEDCOMPONENTS_H
