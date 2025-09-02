#ifndef VECTORDIAGRAMCREATOR_H
#define VECTORDIAGRAMCREATOR_H

#include <QString>
#include <QJsonObject>
#include <QObject>

class VectorDiagramCreator : public QObject
{
    Q_OBJECT
public:
    static QByteArray CreateVectorDiagram(QString rpcOptions, QJsonObject loggedValues);
};

#endif // VECTORDIAGRAMCREATOR_H
