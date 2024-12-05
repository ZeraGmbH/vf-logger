#include "test_loggedcomponents.h"
#include "qtestcase.h"
#include "vl_loggedcomponents.h"
#include <QTest>

QTEST_MAIN(test_loggedcomponents)

using namespace VeinLogger;

static constexpr int entityId = 42;

void test_loggedcomponents::initialComponents()
{
    LoggedComponents components;

    QCOMPARE(components.getEntities().count(), 0); // this will change
}

void test_loggedcomponents::addEntityWithComponentsAndAll()
{
    LoggedComponents components;
    components.addComponent(entityId, "foo");
    QCOMPARE(components.getEntities(), QList<int>() << entityId);
    QCOMPARE(components.getComponents(entityId), QStringList() << "foo");
    QCOMPARE(components.isLoggedComponent(entityId, "foo"), true);
    QCOMPARE(components.isLoggedComponent(entityId, "bar"), false);
    QCOMPARE(components.areAllComponentsStored(entityId), false);

    components.addAllComponents(entityId);
    QCOMPARE(components.getEntities(), QList<int>() << entityId);
    QCOMPARE(components.getComponents(entityId), QStringList()); // all -> empty list
    QCOMPARE(components.isLoggedComponent(entityId, "foo"), true);
    QCOMPARE(components.isLoggedComponent(entityId, "bar"), true);
    QCOMPARE(components.areAllComponentsStored(entityId), true);
}

void test_loggedcomponents::addEntityAllAndWithComponents()
{
    LoggedComponents components;
    components.addAllComponents(entityId);
    QCOMPARE(components.getEntities(), QList<int>() << entityId);
    QCOMPARE(components.getComponents(entityId), QStringList()); // all -> empty list
    QCOMPARE(components.isLoggedComponent(entityId, "foo"), true);
    QCOMPARE(components.isLoggedComponent(entityId, "bar"), true);
    QCOMPARE(components.areAllComponentsStored(entityId), true);

    components.addComponent(entityId, "foo"); // same as all
    QCOMPARE(components.getEntities(), QList<int>() << entityId);
    QCOMPARE(components.getComponents(entityId), QStringList());
    QCOMPARE(components.isLoggedComponent(entityId, "foo"), true);
    QCOMPARE(components.isLoggedComponent(entityId, "bar"), true);
    QCOMPARE(components.areAllComponentsStored(entityId), true);
}

void test_loggedcomponents::withComponentsEntityNameAndMeta()
{
    LoggedComponents components;
    components.addComponent(entityId, "EntityName");
    components.addComponent(entityId, "INF_ModuleInterface");

    QCOMPARE(components.getEntities(), QList<int>() << entityId);
    QCOMPARE(components.getComponents(entityId).count(), 2);
    QCOMPARE(components.getComponents(entityId).contains("EntityName"), true);
    QCOMPARE(components.getComponents(entityId).contains("INF_ModuleInterface"), true);
    QCOMPARE(components.getComponents(entityId).contains("foo"), false);
    QCOMPARE(components.isLoggedComponent(entityId, "EntityName"), true);
    QCOMPARE(components.isLoggedComponent(entityId, "INF_ModuleInterface"), true);
    QCOMPARE(components.isLoggedComponent(entityId, "foo"), false);

    components.addAllComponents(entityId);
    QCOMPARE(components.getComponents(entityId).count(), 0);
    QCOMPARE(components.getComponents(entityId).contains("EntityName"), false);
    QCOMPARE(components.getComponents(entityId).contains("INF_ModuleInterface"), false);
    QCOMPARE(components.getComponents(entityId).contains("foo"), false);
    QCOMPARE(components.isLoggedComponent(entityId, "EntityName"), false);
    QCOMPARE(components.isLoggedComponent(entityId, "INF_ModuleInterface"), false);
    QCOMPARE(components.isLoggedComponent(entityId, "foo"), true);
}

void test_loggedcomponents::allComponentsNoEntityNameAndMeta()
{
    LoggedComponents components;
    components.addAllComponents(entityId);
    components.addComponent(entityId, "INF_ModuleInterface");

    QCOMPARE(components.getEntities(), QList<int>() << entityId);
    QCOMPARE(components.getComponents(entityId).count(), 0);
    QCOMPARE(components.isLoggedComponent(entityId, "EntityName"), false);
    QCOMPARE(components.isLoggedComponent(entityId, "INF_ModuleInterface"), false);
    QCOMPARE(components.isLoggedComponent(entityId, "foo"), true);
}

void test_loggedcomponents::addWithComponentAndClear()
{
    LoggedComponents components;
    components.addComponent(entityId, "foo");
    components.clear();
    QCOMPARE(components.getEntities(), QList<int>());
}

void test_loggedcomponents::addAllAndClear()
{
    LoggedComponents components;
    components.addAllComponents(entityId);
    components.clear();
    QCOMPARE(components.getEntities(), QList<int>());
}
