#ifndef TEST_VECTOR_PAINTING_OPTIONS_H
#define TEST_VECTOR_PAINTING_OPTIONS_H

#include "vectorpaintingoptions.h"
#include <QObject>

class test_vector_painting_options : public QObject
{
    Q_OBJECT
private slots:
    void emptyParams();
    void getDefaultParams();
    void validParamsWithAllOptions();
    void invalidParams();
    // colors
    void colorForInvalidVectorIndex();
    void paramsWithSomeColorsMissing();
    void paramsWithSomeColorsExtra();
    // labels
    void labelForInvalidVectorIndex();
    void paramsWithSomeLabelsMissing();
    void paramsWithSomeLabelsExtra();
    // vector lengths
    void nominalAndMinValuesFromRangeModule();
private:
    void checkIfAllColorsAreSetToDefault(VectorPaintingOptions &options);
    void checkIfAllLabelsAreSetToDefault(VectorPaintingOptions &options);
    void checkIfLengthSettingsAreSetToDefault(VectorPaintingOptions &options);
    QVariantMap readVectorOptionsFromAFile(QString fileName);
};

#endif // TEST_VECTOR_PAINTING_OPTIONS_H
