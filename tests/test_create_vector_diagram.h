#ifndef TEST_CREATE_VECTOR_DIAGRAM_H
#define TEST_CREATE_VECTOR_DIAGRAM_H

#include <QObject>

class test_create_vector_diagram : public QObject
{
    Q_OBJECT
private slots:
    void extractVectorValidIdx();
    void extractVectorInvalidIdx();
    void extractVectorFromJsonWithoutDft();

    void vectorDiagramWithDefaultOptions();
    void vectorDiagramWithInvalidOptions();
    void vectorDiagramWithNoDftValues();
    void vectorDiagramWithCompleteOptionsDftValues();
private:
    QJsonObject readLoggedValues(QString fileName);
    QString readVectorOptions(QString fileName);
};

#endif // TEST_CREATE_VECTOR_DIAGRAM_H
