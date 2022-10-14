#include "test_componentunion.h"
#include "vl_componentunion.h"
#include <QTest>

QTEST_MAIN(test_componentunion)

void test_componentunion::setOneComponentOnEmptySet()
{
    QMap<int, QStringList> map;
    ComponentUnion::uniteComponents(map, 1, QStringList() << "foo");
    QCOMPARE(map.size(), 1);
    QCOMPARE(map[1].size(), 1);
    QCOMPARE(map[1][0], "foo");
}

void test_componentunion::setTwoComponentOnEmptySet()
{
    QMap<int, QStringList> map;
    ComponentUnion::uniteComponents(map, 1, QStringList() << "foo" << "bar");
    QCOMPARE(map.size(), 1);
    QCOMPARE(map[1].size(), 2);
    QCOMPARE(map[1][0], "foo");
    QCOMPARE(map[1][1], "bar");
}

void test_componentunion::setAllComponentOnEmptySet()
{
    QMap<int, QStringList> map;
    ComponentUnion::uniteComponents(map, 1, QStringList());
    QCOMPARE(map.size(), 1);
    QCOMPARE(map[1].size(), 0);
}

void test_componentunion::sequenceOneOneUnequal()
{
    QMap<int, QStringList> map;
    ComponentUnion::uniteComponents(map, 1, QStringList() << "foo");
    ComponentUnion::uniteComponents(map, 1, QStringList() << "bar");
    QCOMPARE(map.size(), 1);
    QCOMPARE(map[1].size(), 2);
    QCOMPARE(map[1][0], "foo");
    QCOMPARE(map[1][1], "bar");
}

void test_componentunion::sequenceOneOneEqual()
{
    QMap<int, QStringList> map;
    ComponentUnion::uniteComponents(map, 1, QStringList() << "foo");
    ComponentUnion::uniteComponents(map, 1, QStringList() << "foo");
    QCOMPARE(map.size(), 1);
    QCOMPARE(map[1].size(), 1);
    QCOMPARE(map[1][0], "foo");
}

void test_componentunion::sequenceOneAll()
{
    QMap<int, QStringList> map;
    ComponentUnion::uniteComponents(map, 1, QStringList() << "foo");
    ComponentUnion::uniteComponents(map, 1, QStringList());
    QCOMPARE(map.size(), 1);
    QCOMPARE(map[1].size(), 0);
}

void test_componentunion::sequenceAllOne()
{
    QMap<int, QStringList> map;
    ComponentUnion::uniteComponents(map, 1, QStringList());
    ComponentUnion::uniteComponents(map, 1, QStringList() << "foo");
    QCOMPARE(map.size(), 1);
    QCOMPARE(map[1].size(), 0);
}

void test_componentunion::twoEntitiesOneAll()
{
    QMap<int, QStringList> map;
    ComponentUnion::uniteComponents(map, 1, QStringList() << "foo");
    ComponentUnion::uniteComponents(map, 2, QStringList());
    QCOMPARE(map.size(), 2);
    QCOMPARE(map[1].size(), 1);
    QCOMPARE(map[1][0], "foo");
    QCOMPARE(map[2].size(), 0);
}
