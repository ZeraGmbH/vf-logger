#ifndef JSONLOGGEDVALUES_H
#define JSONLOGGEDVALUES_H

#include <QJsonObject>
#include <QObject>

class JsonLoggedValues : public QObject
{
    Q_OBJECT
public:
    explicit JsonLoggedValues(QStringList contentsetsList, QObject *parent = nullptr);
    void appendLoggedValues(QString entityId, QString componentName, QVariant value);
    QJsonObject createLoggedValuesJson(QString sessionDeviceName);

private:
    void appendEntitiesOnContentset(QJsonObject &loggedValues, QStringList contentsets, QString sessionDeviceName);

    QStringList m_contentsetsList;
    QJsonObject m_entityCompoValues;
};

#endif // JSONLOGGEDVALUES_H
