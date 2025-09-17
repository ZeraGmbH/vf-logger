#ifndef VECTORDIAGRAMCREATOR_H
#define VECTORDIAGRAMCREATOR_H

#include <QString>
#include <QJsonObject>
#include <QObject>

class VectorDiagramCreator : public QObject
{
    Q_OBJECT
public:
    static QByteArray CreateVectorDiagram(QVariantMap rpcOptions, QJsonObject loggedValues);
    static QVector2D getVector(int idx, QJsonObject loggedValues);
};

#endif // VECTORDIAGRAMCREATOR_H
