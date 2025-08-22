#include "test_vector_painting_options.h"
#include <QTest>

QTEST_MAIN(test_vector_painting_options)

void test_vector_painting_options::invalidJson()
{
    QString input("foo");
    VectorPaintingOptions options;
    QVERIFY(!options.convertJsonParams(input));
    checkIfAllColorsAreSetToDefault(options);
    checkIfAllLabelsAreSetToDefault(options);
    QCOMPARE(options.getVectorStandard(), VectorPaintingOptions::m_defaultVectorStandard);
    QCOMPARE(options.getVectorType(), VectorPaintingOptions::m_defaultVectorType);
    checkIfLengthSettingsAreSetToDefault(options);
    QCOMPARE(options.getVectorStyle(), VectorPaintingOptions::m_defaultStyle);
}

void test_vector_painting_options::getDefaultParams()
{
    VectorPaintingOptions options;
    checkIfAllColorsAreSetToDefault(options);
    checkIfAllLabelsAreSetToDefault(options);
    QCOMPARE(options.getVectorStandard(), VectorPaintingOptions::m_defaultVectorStandard);
    QCOMPARE(options.getVectorType(), VectorPaintingOptions::m_defaultVectorType);
    checkIfLengthSettingsAreSetToDefault(options);
    QCOMPARE(options.getVectorStyle(), VectorPaintingOptions::m_defaultStyle);
}

void test_vector_painting_options::validJsonWithAllOptions()
{
    QFile file(":/vectors-options/complete-options");
    QVERIFY(file.open(QFile::ReadOnly));
    QString params(file.readAll());
    VectorPaintingOptions options;
    QVERIFY(options.convertJsonParams(params));

    QCOMPARE(options.getPhaseColor(0), "#EEff0000");
    QCOMPARE(options.getPhaseColor(1), "#ffe0e000");
    QCOMPARE(options.getPhaseColor(2), "#EE0092ff");
    QCOMPARE(options.getPhaseColor(3), "#EEff0000");
    QCOMPARE(options.getPhaseColor(4), "#ffe0e000");
    QCOMPARE(options.getPhaseColor(5), "#EE0092ff");

    QCOMPARE(options.getPhaseLabel(0), "Van");
    QCOMPARE(options.getPhaseLabel(1), "Vbn");
    QCOMPARE(options.getPhaseLabel(2), "Vcn");
    QCOMPARE(options.getPhaseLabel(3), "Ia");
    QCOMPARE(options.getPhaseLabel(4), "Ib");
    QCOMPARE(options.getPhaseLabel(5), "Ic");

    QCOMPARE(options.getVectorStandard(), VectorSettingsUser::VectorStandard::ANSI);
    QCOMPARE(options.getVectorType(), VectorSettingsUser::VectorType::THREE_PHASE);

    QCOMPARE(options.getNominalSelection(), VectorSettingsLengths::VectorNominals::NOMINAL);
    QCOMPARE(options.getNomVoltage(), 10);
    QCOMPARE(options.getNomCurrent(), 10);
    QCOMPARE(options.getMinVoltage(), 1);
    QCOMPARE(options.getMinCurrent(), 1);

    QCOMPARE(options.getVectorStyle(), VectorSettingsLayout::VectorStyle::WEBSAM);
}

void test_vector_painting_options::colorForInvalidVectorIndex()
{
    VectorPaintingOptions options;
    QCOMPARE(options.getPhaseColor(6), VectorPaintingOptions::m_defaultColor);
}

void test_vector_painting_options::jsonWithSomeColorsMissing()
{
    QFile file(":/vectors-options/missing-some-colors");
    QVERIFY(file.open(QFile::ReadOnly));
    QString params(file.readAll());
    VectorPaintingOptions options;
    QVERIFY(options.convertJsonParams(params));
    QCOMPARE(options.getPhaseColor(0), "#EEff0000");
    QCOMPARE(options.getPhaseColor(1), "#ffe0e000");
    QCOMPARE(options.getPhaseColor(2), VectorPaintingOptions::m_defaultColor);
    QCOMPARE(options.getPhaseColor(3), VectorPaintingOptions::m_defaultColor);
    QCOMPARE(options.getPhaseColor(4), "#ffe0e000");
    QCOMPARE(options.getPhaseColor(5), "#EE0092ff");
}

void test_vector_painting_options::jsonWithSomeColorsExtra()
{
    QFile file(":/vectors-options/some-extra-colors");
    QVERIFY(file.open(QFile::ReadOnly));
    QString params(file.readAll());
    VectorPaintingOptions options;
    QVERIFY(options.convertJsonParams(params));
    QCOMPARE(options.getPhaseColor(0), "#EEff0000");
    QCOMPARE(options.getPhaseColor(1), "#ffe0e000");
    QCOMPARE(options.getPhaseColor(2), "#EE0092ff");
    QCOMPARE(options.getPhaseColor(3), "#EEff0000");
    QCOMPARE(options.getPhaseColor(4), "#ffe0e000");
    QCOMPARE(options.getPhaseColor(5), "#EE0092ff");
}

void test_vector_painting_options::labelForInvalidVectorIndex()
{
    VectorPaintingOptions options;
    QCOMPARE(options.getPhaseLabel(6), "");
}

void test_vector_painting_options::jsonWithSomeLabelsMissing()
{
    QFile file(":/vectors-options/missing-some-labels");
    QVERIFY(file.open(QFile::ReadOnly));
    QString params(file.readAll());
    VectorPaintingOptions options;
    QVERIFY(options.convertJsonParams(params));
    QCOMPARE(options.getPhaseLabel(0), "Van");
    QCOMPARE(options.getPhaseLabel(1), "Vbn");
    QCOMPARE(options.getPhaseLabel(2), VectorPaintingOptions::m_defaultLabels.value(2));
    QCOMPARE(options.getPhaseLabel(3), VectorPaintingOptions::m_defaultLabels.value(3));
    QCOMPARE(options.getPhaseLabel(4), "Ib");
    QCOMPARE(options.getPhaseLabel(5), "Ic");
}

void test_vector_painting_options::jsonWithSomeLabelsExtra()
{
    QFile file(":/vectors-options/some-extra-labels");
    QVERIFY(file.open(QFile::ReadOnly));
    QString params(file.readAll());
    VectorPaintingOptions options;
    QVERIFY(options.convertJsonParams(params));
    QCOMPARE(options.getPhaseLabel(0), "Van");
    QCOMPARE(options.getPhaseLabel(1), "Vbn");
    QCOMPARE(options.getPhaseLabel(2), "Vcn");
    QCOMPARE(options.getPhaseLabel(3), "Ia");
    QCOMPARE(options.getPhaseLabel(4), "Ib");
    QCOMPARE(options.getPhaseLabel(5), "Ic");
}

void test_vector_painting_options::jsonWithMissingSomeLengths()
{
    QFile file(":/vectors-options/missing-current-lengths");
    QVERIFY(file.open(QFile::ReadOnly));
    QString params(file.readAll());
    VectorPaintingOptions options;
    QVERIFY(options.convertJsonParams(params));
    QCOMPARE(options.getNominalSelection(), VectorSettingsLengths::VectorNominals::MAXIMUM);
    QCOMPARE(options.getNomVoltage(), 10);
    QCOMPARE(options.getNomCurrent(), VectorPaintingOptions::m_defaultLengthValue);
    QCOMPARE(options.getMinVoltage(), 1);
    QCOMPARE(options.getMinCurrent(), VectorPaintingOptions::m_defaultLengthValue);
}

void test_vector_painting_options::checkIfAllColorsAreSetToDefault(VectorPaintingOptions &options)
{
    QCOMPARE(options.getPhaseColor(0), VectorPaintingOptions::m_defaultColor);
    QCOMPARE(options.getPhaseColor(1), VectorPaintingOptions::m_defaultColor);
    QCOMPARE(options.getPhaseColor(2), VectorPaintingOptions::m_defaultColor);
    QCOMPARE(options.getPhaseColor(3), VectorPaintingOptions::m_defaultColor);
    QCOMPARE(options.getPhaseColor(4), VectorPaintingOptions::m_defaultColor);
    QCOMPARE(options.getPhaseColor(5), VectorPaintingOptions::m_defaultColor);
}

void test_vector_painting_options::checkIfAllLabelsAreSetToDefault(VectorPaintingOptions &options)
{
    QCOMPARE(options.getPhaseLabel(0), VectorPaintingOptions::m_defaultLabels.value(0));
    QCOMPARE(options.getPhaseLabel(1), VectorPaintingOptions::m_defaultLabels.value(1));
    QCOMPARE(options.getPhaseLabel(2), VectorPaintingOptions::m_defaultLabels.value(2));
    QCOMPARE(options.getPhaseLabel(3), VectorPaintingOptions::m_defaultLabels.value(3));
    QCOMPARE(options.getPhaseLabel(4), VectorPaintingOptions::m_defaultLabels.value(4));
    QCOMPARE(options.getPhaseLabel(5), VectorPaintingOptions::m_defaultLabels.value(5));
}

void test_vector_painting_options::checkIfLengthSettingsAreSetToDefault(VectorPaintingOptions &options)
{
    QCOMPARE(options.getNominalSelection(), VectorPaintingOptions::m_defaultNominalSelection);
    QCOMPARE(options.getNomVoltage(), VectorPaintingOptions::m_defaultLengthValue);
    QCOMPARE(options.getNomCurrent(), VectorPaintingOptions::m_defaultLengthValue);
    QCOMPARE(options.getMinVoltage(), VectorPaintingOptions::m_defaultLengthValue);
    QCOMPARE(options.getMinCurrent(), VectorPaintingOptions::m_defaultLengthValue);
}
