#include "test_create_vector_diagram.h"
#include "vectordiagramcreator.h"
#include <svgfuzzycompare.h>
#include <testloghelpers.h>
#include <QJsonDocument>
#include <QTest>

QTEST_MAIN(test_create_vector_diagram)

void test_create_vector_diagram::extractVectorValidIdx()
{
    QJsonObject actualValues = readLoggedValues(":/logged-values/dftValues.json");
    QVector2D expectedVector(169.706, 0.0);
    QVector2D actualVector = VectorDiagramCreator::getVector(0, actualValues);
    QCOMPARE(expectedVector, actualVector);
}

void test_create_vector_diagram::extractVectorInvalidIdx()
{
    QJsonObject actualValues = readLoggedValues(":/logged-values/dftValues.json");
    QVector2D expectedVector(0.0, 0.0);
    QVector2D actualVector = VectorDiagramCreator::getVector(6, actualValues);
    QCOMPARE(expectedVector, actualVector);
}

void test_create_vector_diagram::extractVectorFromJsonWithoutDft()
{
    QJsonObject actualValues = readLoggedValues(":/logged-values/rangeValues.json");
    QVector2D expectedVector(0.0, 0.0);
    QVector2D actualVector = VectorDiagramCreator::getVector(0, actualValues);
    QCOMPARE(expectedVector, actualVector);
}

void test_create_vector_diagram::vectorDiagramWithDefaultOptions()
{
    QJsonObject dftValues = readLoggedValues(":/logged-values/dftValues.json");
    QString expected = TestLogHelpers::loadFile(":/vector-diagram-svgs/vectorDiagramWithDefaultOptionsValidDftValues.svg");
    QString dumped = VectorDiagramCreator::CreateVectorDiagram(QVariantMap(), dftValues);
    SvgFuzzyCompare compare;
    bool ok = compare.compareXml(dumped, expected);
    if(!ok)
        TestLogHelpers::compareAndLogOnDiff(expected, dumped);
    QVERIFY(ok);
}

void test_create_vector_diagram::vectorDiagramWithInvalidOptions()
{
    QJsonObject dftValues = readLoggedValues(":/logged-values/dftValues.json");
    QVariantMap options = readVectorOptionsFromAFile(":/vectors-options/missing-some-colors");
    QString expected = TestLogHelpers::loadFile(":/vector-diagram-svgs/vectorDiagramWithInvalidOptionsValidDftValues.svg");
    QString dumped = VectorDiagramCreator::CreateVectorDiagram(options, dftValues);
    SvgFuzzyCompare compare;
    bool ok = compare.compareXml(dumped, expected);
    if(!ok)
        TestLogHelpers::compareAndLogOnDiff(expected, dumped);
    QVERIFY(ok);
}

void test_create_vector_diagram::vectorDiagramWithNoDftValues()
{
    QJsonObject loggedValues = readLoggedValues(":/logged-values/rangeValues.json");
    QVariantMap options = readVectorOptionsFromAFile(":/vectors-options/complete-options");
    QString expected = TestLogHelpers::loadFile(":/vector-diagram-svgs/vectorDiagramWithCustomOptionsNoDftValues.svg");
    QString dumped = VectorDiagramCreator::CreateVectorDiagram(options, loggedValues);
    SvgFuzzyCompare compare;
    bool ok = compare.compareXml(dumped, expected);
    if(!ok)
        TestLogHelpers::compareAndLogOnDiff(expected, dumped);
    QVERIFY(ok);
}

void test_create_vector_diagram::vectorDiagramWithCompleteOptionsDftValues()
{
    QJsonObject dftRangeValues = readLoggedValues(":/logged-values/dftRangeValues.json");
    QVariantMap options = readVectorOptionsFromAFile(":/vectors-options/complete-options");
    QString expected = TestLogHelpers::loadFile(":/vector-diagram-svgs/vectorDiagramWithCustomOptionsValidDftValues.svg");
    QString dumped = VectorDiagramCreator::CreateVectorDiagram(options, dftRangeValues);
    SvgFuzzyCompare compare;
    bool ok = compare.compareXml(dumped, expected);
    if(!ok)
        TestLogHelpers::compareAndLogOnDiff(expected, dumped);
    QVERIFY(ok);
}

QJsonObject test_create_vector_diagram::readLoggedValues(QString fileName)
{
    QFile file(fileName);
    file.open(QFile::ReadOnly);
    QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    return document.object();
}

QVariantMap test_create_vector_diagram::readVectorOptionsFromAFile(QString fileName)
{
    QFile fileOptions(fileName);
    fileOptions.open(QFile::ReadOnly);
    QJsonDocument doc = QJsonDocument::fromJson(fileOptions.readAll());
    return doc.object().toVariantMap();
}
