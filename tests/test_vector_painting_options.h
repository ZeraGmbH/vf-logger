#ifndef TEST_VECTOR_PAINTING_OPTIONS_H
#define TEST_VECTOR_PAINTING_OPTIONS_H

#include "vectorpaintingoptions.h"
#include <QObject>

class test_vector_painting_options : public QObject
{
    Q_OBJECT
private slots:
    void invalidJson();
    void getDefaultParams();
    void validJsonWithAllOptions();
    // colors
    void colorForInvalidVectorIndex();
    void jsonWithSomeColorsMissing();
    void jsonWithSomeColorsExtra();
    // labels
    void labelForInvalidVectorIndex();
    void jsonWithSomeLabelsMissing();
    void jsonWithSomeLabelsExtra();
    // vector lengths
    void nominalAndMinValuesFromRangeModule();
private:
    void checkIfAllColorsAreSetToDefault(VectorPaintingOptions &options);
    void checkIfAllLabelsAreSetToDefault(VectorPaintingOptions &options);
    void checkIfLengthSettingsAreSetToDefault(VectorPaintingOptions &options);
};

#endif // TEST_VECTOR_PAINTING_OPTIONS_H
