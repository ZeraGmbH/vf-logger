#ifndef TEST_COMPONENTUNION_H
#define TEST_COMPONENTUNION_H

#include <QObject>

class test_componentunion : public QObject
{
    Q_OBJECT
private slots:
    void setOneComponentOnEmptySet();
    void setTwoComponentOnEmptySet();
    void setAllComponentOnEmptySet();

    void sequenceOneOneUnequal();
    void sequenceOneOneEqual();

    void sequenceOneAll();
    void sequenceAllOne();

    void twoEntitiesOneAll();
};

#endif // TEST_COMPONENTUNION_H
