#include "vectordiagramcreator.h"
#include "vectorpaintingoptions.h"
#include <vectorpaintcontroller.h>
#include <vectortosvgpainter.h>
#include <vectorconstants.h>

static constexpr const char* DftModule = "1050";
constexpr int clipLen = 2000;

QByteArray VectorDiagramCreator::CreateVectorDiagram(QString rpcOptions, QJsonObject loggedValues)
{
    VectorPaintingOptions paintingOptions;
    paintingOptions.convertJsonParams(rpcOptions);
    paintingOptions.calculateNominalAndMinValues(loggedValues);

    VectorPaintController paintController;
    for (int i=0; i<VectorConstants::COUNT_VECTORS; i++) {
        paintController.setVector(i, getVector(i, loggedValues));
        paintController.setVectorColor(i, paintingOptions.getPhaseColor(i));
        paintController.setVectorLabel(i, paintingOptions.getPhaseLabel(i));
    }

    paintController.getVectorSettings()->m_user.setVectorStandard(paintingOptions.getVectorStandard());
    paintController.getVectorSettings()->m_user.setVectorType(paintingOptions.getVectorType());

    paintController.getVectorSettings()->m_lengths.setNominalSelection(paintingOptions.getNominalSelection());
    paintController.getVectorSettings()->m_lengths.setNomVoltage(paintingOptions.getNomVoltage());
    paintController.getVectorSettings()->m_lengths.setNomCurrent(paintingOptions.getNomCurrent());
    paintController.getVectorSettings()->m_lengths.setMinVoltage(paintingOptions.getMinVoltage());
    paintController.getVectorSettings()->m_lengths.setMinCurrent(paintingOptions.getMinCurrent());

    paintController.getVectorSettings()->m_layout.setVectorStyle(paintingOptions.getVectorStyle());

    VectorToSvgPainter painter(clipLen, clipLen);
    return painter.paintByteArray(&paintController);
}

QVector2D VectorDiagramCreator::getVector(int idx, QJsonObject loggedValues)
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
