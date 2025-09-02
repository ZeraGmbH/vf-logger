#ifndef TEST_VECTOR_ACTUAL_VALUE_EXTRACTOR_H
#define TEST_VECTOR_ACTUAL_VALUE_EXTRACTOR_H

#include <QObject>

class test_vector_actual_value_extractor : public QObject
{
    Q_OBJECT
private slots:
    void extractValidIdx();
    void extractInvalidIdx();
    void extractFromJsonWithoutDft();
};

#endif // TEST_VECTOR_ACTUAL_VALUE_EXTRACTOR_H
