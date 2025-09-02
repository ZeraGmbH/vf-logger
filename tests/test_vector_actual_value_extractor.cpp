#include "test_vector_actual_value_extractor.h"
#include "vectoractualvalueextractor.h"
#include <QJsonDocument>
#include <QTest>

QTEST_MAIN(test_vector_actual_value_extractor)

void test_vector_actual_value_extractor::extractValidIdx()
{
    QFile file(":/logged-values/dftValues.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    QJsonObject actualValues = document.object();

    VectorActualValueExtractor extrator;
    QVector2D expectedVector(169.706, 0.0);
    QVector2D actualVector = extrator.getVector(0, actualValues);
    QCOMPARE(expectedVector, actualVector);
}

void test_vector_actual_value_extractor::extractInvalidIdx()
{
    QFile file(":/logged-values/dftValues.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    QJsonObject actualValues = document.object();

    VectorActualValueExtractor extrator;
    QVector2D expectedVector(0.0, 0.0);
    QVector2D actualVector = extrator.getVector(6, actualValues);
    QCOMPARE(expectedVector, actualVector);
}

void test_vector_actual_value_extractor::extractFromJsonWithoutDft()
{
    QFile file(":/logged-values/rangeValues.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    QJsonObject actualValues = document.object();

    VectorActualValueExtractor extrator;
    QVector2D expectedVector(0.0, 0.0);
    QVector2D actualVector = extrator.getVector(0, actualValues);
    QCOMPARE(expectedVector, actualVector);
}

