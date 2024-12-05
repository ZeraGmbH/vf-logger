#ifndef TEST_LOGGEDCOMPONENTS_H
#define TEST_LOGGEDCOMPONENTS_H

#include <QObject>

class test_loggedcomponents : public QObject
{
    Q_OBJECT
private slots:
    void initialComponents();
    void addEntityWithComponentsAndAll();
    void addEntityAllAndWithComponents();

    void withComponentsEntityNameAndMeta();
    void allComponentsNoEntityNameAndMeta();

    void addWithComponentAndClear();
    void addAllAndClear();
};

#endif // TEST_LOGGEDCOMPONENTS_H
