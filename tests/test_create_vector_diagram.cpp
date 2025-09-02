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
    const QString fileBase = QString(QTest::currentTestFunction()) + ".svg";
    QString expected = TestLogHelpers::loadFile(QString(":/vector-diagram-svgs/") + fileBase);
    QString dumped = VectorDiagramCreator::CreateVectorDiagram("", dftValues);
    SvgFuzzyCompare compare;
    bool ok = compare.compareXml(dumped, expected);
    if(!ok)
        TestLogHelpers::compareAndLogOnDiff(expected, dumped);
    QVERIFY(ok);
}

void test_create_vector_diagram::vectorDiagramWithInvalidOptions()
{
    QJsonObject dftValues = readLoggedValues(":/logged-values/dftValues.json");
    QString options = readVectorOptions(":/vectors-options/missing-some-colors");
    const QString fileBase = QString(QTest::currentTestFunction()) + ".svg";
    QString expected = TestLogHelpers::loadFile(QString(":/vector-diagram-svgs/") + fileBase);
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
    QString options = readVectorOptions(":/vectors-options/complete-options");
    const QString fileBase = QString(QTest::currentTestFunction()) + ".svg";
    QString expected = TestLogHelpers::loadFile(QString(":/vector-diagram-svgs/") + fileBase);
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
    QString options = readVectorOptions(":/vectors-options/complete-options");
    const QString fileBase = QString(QTest::currentTestFunction()) + ".svg";
    QString expected = TestLogHelpers::loadFile(QString(":/vector-diagram-svgs/") + fileBase);
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

QString test_create_vector_diagram::readVectorOptions(QString fileName)
{
    QFile fileOptions(fileName);
    fileOptions.open(QFile::ReadOnly);
    return fileOptions.readAll();
}
