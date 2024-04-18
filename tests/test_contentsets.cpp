#include "test_contentsets.h"
#include "testdumpreporter.h"
#include <timemachineobject.h>
#include <QTest>

QTEST_MAIN(test_contentsets)

void test_contentsets::init()
{
    m_testSystem.setupServer();
}

void test_contentsets::cleanup()
{
    m_testSystem.cleanup();
}

void test_contentsets::contentSetsSelectValid()
{
    m_testSystem.setComponent(dataLoggerEntityId, "currentContentSets", "TestSet1"); // this is a bug - stored value must be a list!!!

    QFile file(":/vein-dumps/dumpSetContentSetValidByString.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_contentsets::contentSetsSelectInvalid()
{
    m_testSystem.setComponent(dataLoggerEntityId, "currentContentSets", "foo");

    QFile file(":/vein-dumps/dumpSetContentSetInvalid.json"); // another bug: we expect dumpInitial.json
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_contentsets::contentSetsSelectValidList()
{
    m_testSystem.setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "TestSet1");

    QFile file(":/vein-dumps/dumpSetContentValid.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_contentsets::contentSetsSelectValidListTwo()
{
    m_testSystem.setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "TestSet1" << "TestSet2");

    QFile file(":/vein-dumps/dumpSetContentValidTwo.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_contentsets::contentSetsSelectValidListTwoSame()
{
    m_testSystem.setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "TestSet1" << "TestSet1");

    QFile file(":/vein-dumps/dumpSetContentValidTwoSame.json"); // we would not expect two identical...
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_contentsets::contentSetsSelectValidListAll()
{
    m_testSystem.setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "ZeraAll");

    QFile file(":/vein-dumps/dumpSetContentAll.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    // Just to remember: All Entities stored are completely independent of context set configured
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_contentsets::contentSetsSelectValidListSequence()
{
    m_testSystem.setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "TestSet2");
    m_testSystem.setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "TestSet1");

    QFile file(":/vein-dumps/dumpSetContentValid.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

