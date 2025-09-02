#include "test_create_vector_diagram.h"
#include "vectordiagramcreator.h"
#include <svgfuzzycompare.h>
#include <testloghelpers.h>
#include <QJsonDocument>
#include <QTest>

QTEST_MAIN(test_create_vector_diagram)

void test_create_vector_diagram::extractVectorValidIdx()
{
    QFile file(":/logged-values/dftValues.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    QJsonObject actualValues = document.object();

    QVector2D expectedVector(169.706, 0.0);
    QVector2D actualVector = VectorDiagramCreator::getVector(0, actualValues);
    QCOMPARE(expectedVector, actualVector);
}

void test_create_vector_diagram::extractVectorInvalidIdx()
{
    QFile file(":/logged-values/dftValues.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    QJsonObject actualValues = document.object();

    QVector2D expectedVector(0.0, 0.0);
    QVector2D actualVector = VectorDiagramCreator::getVector(6, actualValues);
    QCOMPARE(expectedVector, actualVector);
}

void test_create_vector_diagram::extractVectorFromJsonWithoutDft()
{
    QFile file(":/logged-values/rangeValues.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    QJsonObject actualValues = document.object();

    QVector2D expectedVector(0.0, 0.0);
    QVector2D actualVector = VectorDiagramCreator::getVector(0, actualValues);
    QCOMPARE(expectedVector, actualVector);
}

void test_create_vector_diagram::vectorDiagramWithDefaultOptions()
{
    QFile file(":/logged-values/dftValues.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    QJsonObject dftValues = document.object();

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
    QFile fileLoggedValues(":/logged-values/dftValues.json");
    QVERIFY(fileLoggedValues.open(QFile::ReadOnly));
    QJsonDocument document = QJsonDocument::fromJson(fileLoggedValues.readAll());
    QJsonObject dftValues = document.object();

    QFile fileOptions(":/vectors-options/missing-some-colors");
    QVERIFY(fileOptions.open(QFile::ReadOnly));
    QString options = fileOptions.readAll();

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
    QFile fileLoggedValues(":/logged-values/rangeValues.json");
    QVERIFY(fileLoggedValues.open(QFile::ReadOnly));
    QJsonDocument document = QJsonDocument::fromJson(fileLoggedValues.readAll());
    QJsonObject loggedValues = document.object();

    QFile fileOptions(":/vectors-options/complete-options");
    QVERIFY(fileOptions.open(QFile::ReadOnly));
    QString options = fileOptions.readAll();

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
    QFile fileLoggedValues(":/logged-values/dftRangeValues.json");
    QVERIFY(fileLoggedValues.open(QFile::ReadOnly));
    QJsonDocument document = QJsonDocument::fromJson(fileLoggedValues.readAll());
    QJsonObject dftRangeValues = document.object();

    QFile fileOptions(":/vectors-options/complete-options");
    QVERIFY(fileOptions.open(QFile::ReadOnly));
    QString options = fileOptions.readAll();

    const QString fileBase = QString(QTest::currentTestFunction()) + ".svg";
    QString expected = TestLogHelpers::loadFile(QString(":/vector-diagram-svgs/") + fileBase);
    QString dumped = VectorDiagramCreator::CreateVectorDiagram(options, dftRangeValues);
    SvgFuzzyCompare compare;
    bool ok = compare.compareXml(dumped, expected);
    if(!ok)
        TestLogHelpers::compareAndLogOnDiff(expected, dumped);
    QVERIFY(ok);
}
