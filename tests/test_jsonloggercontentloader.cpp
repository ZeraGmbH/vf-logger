#include "test_jsonloggercontentloader.h"
#include "contentsetsotherfromcontentsetsconfig.h"
#include <QFileInfo>
#include <QTest>

QTEST_MAIN(test_jsonloggercontentloader)

void test_jsonloggercontentloader::noSessionSetEmptyAvailableContentSets()
{
    ContentSetsOtherFromContentSetsConfig loader;
    QVERIFY(loader.getAvailableContentSets().isEmpty());
}

void test_jsonloggercontentloader::testSimpleAvailContentSets()
{
    ContentSetsOtherFromContentSetsConfig loader;
    loader.setConfigFileDir(JSON_TEST_DIR);
    loader.setModmanSession("simple-valid.json");
    QStringList cs = loader.getAvailableContentSets();
    cs.sort();
    QCOMPARE(cs.size(), 3);
    QCOMPARE(cs[0], "ZeraActualValues");
    QCOMPARE(cs[1], "ZeraDCReference");
    QCOMPARE(cs[2], "ZeraHarmonics");
}

void test_jsonloggercontentloader::testSimpleEntityComponentsNoMatch()
{
    ContentSetsOtherFromContentSetsConfig loader;
    loader.setConfigFileDir(JSON_TEST_DIR);
    loader.setModmanSession("simple-valid.json");
    QMap<int, QStringList> entityComponentMap = loader.getEntityComponents("foo");
    QVERIFY(entityComponentMap.isEmpty());
}

void test_jsonloggercontentloader::testSimpleEntityComponentsActual()
{
    ContentSetsOtherFromContentSetsConfig loader;
    loader.setConfigFileDir(JSON_TEST_DIR);
    loader.setModmanSession("simple-valid.json");
    QMap<int, QStringList> entityComponentMap = loader.getEntityComponents("ZeraActualValues");
    QCOMPARE(entityComponentMap.count(), 2);
    QCOMPARE(entityComponentMap[1040].count(), 0);
    QCOMPARE(entityComponentMap[1050].count(), 0);
}

void test_jsonloggercontentloader::testSimpleEntityComponentsHarmonics()
{
    ContentSetsOtherFromContentSetsConfig loader;
    loader.setConfigFileDir(JSON_TEST_DIR);
    loader.setModmanSession("simple-valid.json");
    QMap<int, QStringList> entityComponentMap = loader.getEntityComponents("ZeraHarmonics");
    QCOMPARE(entityComponentMap.count(), 2);
    QCOMPARE(entityComponentMap[1110].count(), 0);
    QCOMPARE(entityComponentMap[1060].count(), 0);
}

void test_jsonloggercontentloader::testSimpleEntityComponentsDcRef()
{
    ContentSetsOtherFromContentSetsConfig loader;
    loader.setConfigFileDir(JSON_TEST_DIR);
    loader.setModmanSession("simple-valid.json");
    QMap<int, QStringList> entityComponentMap = loader.getEntityComponents("ZeraDCReference");
    QCOMPARE(entityComponentMap.count(), 1);
    QCOMPARE(entityComponentMap[1050].count(), 3);
    QStringList components = entityComponentMap[1050];
    components.sort();
    QCOMPARE(components[0], "ACT_DFTPN1");
    QCOMPARE(components[1], "ACT_DFTPN2");
    QCOMPARE(components[2], "ACT_DFTPN3");
}
