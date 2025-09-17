#include "test_vector_painting_options.h"
#include <vs_dumpjson.h>
#include <QJsonDocument>
#include <QTest>
#include <cmath>

QTEST_MAIN(test_vector_painting_options)

void test_vector_painting_options::emptyJson()
{
    VectorPaintingOptions options;
    QVERIFY(options.convertJsonParams(QVariantMap()));
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
    QVariantMap params = readVectorOptionsFromAFile(":/vectors-options/complete-options");
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
    QCOMPARE(options.getVectorType(), VectorSettingsUser::VectorType::TRIANGLE);
    QCOMPARE(options.getNominalSelection(), VectorSettingsLengths::VectorNominals::NOMINAL);
    QCOMPARE(options.getVectorStyle(), VectorSettingsLayout::VectorStyle::WEBSAM);
}

void test_vector_painting_options::colorForInvalidVectorIndex()
{
    VectorPaintingOptions options;
    QCOMPARE(options.getPhaseColor(6), VectorPaintingOptions::m_defaultColor);
}

void test_vector_painting_options::jsonWithSomeColorsMissing()
{
    QVariantMap params = readVectorOptionsFromAFile(":/vectors-options/missing-some-colors");
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
    QVariantMap params = readVectorOptionsFromAFile(":/vectors-options/some-extra-colors");
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
    QVariantMap params = readVectorOptionsFromAFile(":/vectors-options/missing-some-labels");
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
    QVariantMap params = readVectorOptionsFromAFile(":/vectors-options/some-extra-labels");
    VectorPaintingOptions options;
    QVERIFY(options.convertJsonParams(params));
    QCOMPARE(options.getPhaseLabel(0), "Van");
    QCOMPARE(options.getPhaseLabel(1), "Vbn");
    QCOMPARE(options.getPhaseLabel(2), "Vcn");
    QCOMPARE(options.getPhaseLabel(3), "Ia");
    QCOMPARE(options.getPhaseLabel(4), "Ib");
    QCOMPARE(options.getPhaseLabel(5), "Ic");
}

void test_vector_painting_options::nominalAndMinValuesFromRangeModule()
{
    QFile file(":/logged-values/rangeValues.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    QJsonObject loggedValues = document.object();

    VectorPaintingOptions options;
    options.calculateNominalAndMinValues(loggedValues);
    float expectedNomVoltage = 250 * sqrt(2);
    QCOMPARE(VeinStorage::DumpJson::formatDouble(options.getNomVoltage()), VeinStorage::DumpJson::formatDouble(expectedNomVoltage));
    float expectedNomCurrent = 10 * sqrt(2);
    QCOMPARE(VeinStorage::DumpJson::formatDouble(options.getNomCurrent()), VeinStorage::DumpJson::formatDouble(expectedNomCurrent));
    float expectedMinVoltage = expectedNomVoltage * 0.009;
    QCOMPARE(VeinStorage::DumpJson::formatDouble(options.getMinVoltage()), VeinStorage::DumpJson::formatDouble(expectedMinVoltage));
    float expectedMinCurrent = expectedNomCurrent * 0.009;
    QCOMPARE(VeinStorage::DumpJson::formatDouble(options.getMinCurrent()), VeinStorage::DumpJson::formatDouble(expectedMinCurrent));
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

QVariantMap test_vector_painting_options::readVectorOptionsFromAFile(QString fileName)
{
    QFile fileOptions(fileName);
    fileOptions.open(QFile::ReadOnly);
    QJsonDocument doc = QJsonDocument::fromJson(fileOptions.readAll());
    return doc.object().toVariantMap();
}
