#include "vectoractualvalueextractor.h"
#include <vectorconstants.h>

static constexpr const char* DftModule = "1050";

QVector2D VectorActualValueExtractor::getVector(int idx, QJsonObject loggedValues)
{
    QVector2D nullVector(0.0, 0.0);
    if(idx >= VectorConstants::COUNT_VECTORS) {
        qWarning("VectorActualValueExtractor::getVector idx is out of limit.");
        return nullVector;
    }

    if(!loggedValues.contains(DftModule))
        return nullVector;

    QJsonObject dftModuleJson = loggedValues.value(DftModule).toObject();
    QString channel = QString::number(idx + 1);
    QStringList vectorPointsStr = dftModuleJson.value("ACT_DFTPN" + channel).toString().split(";");
    return QVector2D(vectorPointsStr.at(0).toFloat(), vectorPointsStr.at(1).toFloat());
}
