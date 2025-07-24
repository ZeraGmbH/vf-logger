#include "test_loggercontentsessionloader.h"
#include "contentsetszeraallfrommodmansessions.h"
#include <QTest>

QTEST_MAIN(test_loggercontentsessionloader)

void test_loggercontentsessionloader::noSessionSetEmptyAvailableContentSets()
{
    ContentSetsZeraAllFromModmanSessions loader;
    QVERIFY(loader.getAvailableContentSets().isEmpty());
}

void test_loggercontentsessionloader::testSimpleAvailContentSets()
{
    ContentSetsZeraAllFromModmanSessions loader;
    loader.setConfigFileDir(JSON_TEST_DIR);
    loader.setModmanSession("simple-valid-session.json");
    QStringList cs = loader.getAvailableContentSets();
    QCOMPARE(cs.size(), 1);
    QCOMPARE(cs[0], "ZeraAll");
}

void test_loggercontentsessionloader::testSimpleEntityComponentsNoMatch()
{
    ContentSetsZeraAllFromModmanSessions loader;
    loader.setConfigFileDir(JSON_TEST_DIR);
    loader.setModmanSession("simple-valid-session.json");
    QMap<int, QStringList> entityComponentMap = loader.getEntityComponents("foo");
    QVERIFY(entityComponentMap.isEmpty());
}

void test_loggercontentsessionloader::testSimpleEntityComponentsAll()
{
    ContentSetsZeraAllFromModmanSessions loader;
    loader.setConfigFileDir(JSON_TEST_DIR);
    loader.setModmanSession("simple-valid-session.json");
    QMap<int, QStringList> entityComponentMap = loader.getEntityComponents("ZeraAll");
    QCOMPARE(entityComponentMap.count(), 3);
    QCOMPARE(entityComponentMap[1150].count(), 0);
    QCOMPARE(entityComponentMap[1000].count(), 0);
    QCOMPARE(entityComponentMap[1020].count(), 0);
}
