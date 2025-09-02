#ifndef TEST_CREATE_VECTOR_DIAGRAM_H
#define TEST_CREATE_VECTOR_DIAGRAM_H

#include <QObject>

class test_create_vector_diagram : public QObject
{
    Q_OBJECT
private slots:
    void vectorDiagramWithDefaultOptions();
    void vectorDiagramWithInvalidOptions();
    void vectorDiagramWithNoDftValues();
    void vectorDiagramWithCompleteOptionsDftValues();
};

#endif // TEST_CREATE_VECTOR_DIAGRAM_H
