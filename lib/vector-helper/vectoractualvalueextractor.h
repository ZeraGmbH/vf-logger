#ifndef VECTORACTUALVALUEEXTRACTOR_H
#define VECTORACTUALVALUEEXTRACTOR_H

#include <QVector2D>
#include <QJsonObject>
#include <QObject>

class VectorActualValueExtractor : public QObject
{
    Q_OBJECT
public:
    static QVector2D getVector(int idx, QJsonObject loggedValues);
};

#endif // VECTORACTUALVALUEEXTRACTOR_H
