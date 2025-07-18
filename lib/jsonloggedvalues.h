#ifndef JSONLOGGEDVALUES_H
#define JSONLOGGEDVALUES_H

#include <QJsonObject>
#include <QObject>

class JsonLoggedValues : public QObject
{
    Q_OBJECT
public:
    explicit JsonLoggedValues(QStringList contentsetsList);
    void appendLoggedValues(QString entityId, QString componentName, QVariant value);
    QJsonObject createLoggedValuesJson(QString sessionDeviceName);

private:
    QStringList m_contentsetsList;
    QJsonObject m_entityCompoValues;
    QList<int> m_entitiesWithAllComponentsStoredAlways;
};

#endif // JSONLOGGEDVALUES_H
