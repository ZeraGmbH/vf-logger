#include "vectordiagramcreator.h"
#include "vectoractualvalueextractor.h"
#include "vectorpaintingoptions.h"
#include <vectorpaintcontroller.h>
#include <vectortosvgpainter.h>
#include <vectorconstants.h>

constexpr int clipLen = 2000;

QByteArray VectorDiagramCreator::CreateVectorDiagram(QString rpcOptions, QJsonObject loggedValues)
{
    VectorPaintingOptions paintingOptions;
    paintingOptions.convertJsonParams(rpcOptions);
    paintingOptions.calculateNominalAndMinValues(loggedValues);

    VectorPaintController paintController;
    for (int i=0; i<VectorConstants::COUNT_VECTORS; i++) {
        paintController.setVector(i, VectorActualValueExtractor::getVector(i, loggedValues));
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
