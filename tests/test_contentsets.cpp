#include "test_contentsets.h"
#include "loggerstatictexts.h"
#include "contentsetsotherfromcontentsetsconfig.h"
#include "contentsetszeraallfrommodmansessions.h"
#include "testloghelpers.h"
#include "loggercontentsetconfig.h"
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

void test_contentsets::contentSetsJsonEnviornmentSetTwice()
{
    QCOMPARE(VeinLogger::LoggerContentSetConfig::getConfigEnvironment().count(), 2);

    VeinLogger::LoggerContentSetConfig::setJsonEnvironment(":/contentsets/", std::make_shared<ContentSetsOtherFromContentSetsConfig>());
    QCOMPARE(VeinLogger::LoggerContentSetConfig::getConfigEnvironment().count(), 2);

    VeinLogger::LoggerContentSetConfig::setJsonEnvironment(":/sessions/", std::make_shared<ContentSetsZeraAllFromModmanSessions>());
    QCOMPARE(VeinLogger::LoggerContentSetConfig::getConfigEnvironment().count(), 2);
}

void test_contentsets::contentSetsSelectValidStringToBeFixed()
{
    m_testSystem.setComponent(dataLoggerEntityId, LoggerStaticTexts::s_currentContentSetsComponentName, "TestSet1"); // this is a bug - stored value must be a list!!!

    QFile file(":/vein-dumps/dumpSetContentSetValidByString.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiffJson(jsonExpected, jsonDumped));
}

void test_contentsets::contentSetsSelectInvalid()
{
    m_testSystem.setComponent(dataLoggerEntityId, LoggerStaticTexts::s_currentContentSetsComponentName, "foo");

    QFile file(":/vein-dumps/dumpSetContentSetInvalid.json"); // another bug: we expect dumpInitial.json
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiffJson(jsonExpected, jsonDumped));
}

void test_contentsets::contentSetsSelectValid()
{
    m_testSystem.setComponent(dataLoggerEntityId, LoggerStaticTexts::s_currentContentSetsComponentName, QVariantList() << "TestSet1");

    QFile file(":/vein-dumps/dumpSetContentValid.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiffJson(jsonExpected, jsonDumped));
}

void test_contentsets::contentSetsSelectValidTwo()
{
    m_testSystem.setComponent(dataLoggerEntityId, LoggerStaticTexts::s_currentContentSetsComponentName, QVariantList() << "TestSet1" << "TestSet2");

    QFile file(":/vein-dumps/dumpSetContentValidTwo.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiffJson(jsonExpected, jsonDumped));
}

void test_contentsets::contentSetsSelectValidTwoSame()
{
    m_testSystem.setComponent(dataLoggerEntityId, LoggerStaticTexts::s_currentContentSetsComponentName, QVariantList() << "TestSet1" << "TestSet1");

    QFile file(":/vein-dumps/dumpSetContentValidTwoSame.json"); // we would not expect two identical...
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiffJson(jsonExpected, jsonDumped));
}

void test_contentsets::sameContentSetDifferentSession()
{
    m_testSystem.setComponent(dataLoggerEntityId, LoggerStaticTexts::s_currentContentSetsComponentName, QVariantList() << "TestSet1");
    QVariantMap loggedComponents = m_testSystem.getValueOfComponent(dataLoggerEntityId, "LoggedComponents").toMap();
    QCOMPARE(loggedComponents.count(), 1);
    QCOMPARE(loggedComponents.keys().at(0), "10");

    m_testSystem.changeSession();
    QVariant currentContentSets = m_testSystem.getValueOfComponent(dataLoggerEntityId, LoggerStaticTexts::s_currentContentSetsComponentName);
    QVERIFY(currentContentSets.toStringList().isEmpty());
    loggedComponents = m_testSystem.getValueOfComponent(dataLoggerEntityId, "LoggedComponents").toMap();
    QVERIFY(loggedComponents.isEmpty());

    m_testSystem.setComponent(dataLoggerEntityId, LoggerStaticTexts::s_currentContentSetsComponentName, QVariantList() << "TestSet1");
    loggedComponents = m_testSystem.getValueOfComponent(dataLoggerEntityId, "LoggedComponents").toMap();
    QCOMPARE(loggedComponents.count(), 1);
    QCOMPARE(loggedComponents.keys().at(0), "11");
}

void test_contentsets::contentSetsSelectValidAll()
{
    m_testSystem.setComponent(dataLoggerEntityId, LoggerStaticTexts::s_currentContentSetsComponentName, QVariantList() << "ZeraAll");

    QFile file(":/vein-dumps/dumpSetContentAll.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    // Just to remember: All Entities stored are completely independent of context set configured
    QVERIFY(TestLogHelpers::compareAndLogOnDiffJson(jsonExpected, jsonDumped));
}

void test_contentsets::contentSetsSelectValidSequence()
{
    m_testSystem.setComponent(dataLoggerEntityId, LoggerStaticTexts::s_currentContentSetsComponentName, QVariantList() << "TestSet2");
    m_testSystem.setComponent(dataLoggerEntityId, LoggerStaticTexts::s_currentContentSetsComponentName, QVariantList() << "TestSet1");

    QFile file(":/vein-dumps/dumpSetContentValid.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiffJson(jsonExpected, jsonDumped));
}

void test_contentsets::contentSetsSelectValid3SessionChange()
{
    m_testSystem.changeSession();
    m_testSystem.setComponent(dataLoggerEntityId, LoggerStaticTexts::s_currentContentSetsComponentName, QVariantList() << "TestSet3");

    QFile file(":/vein-dumps/dumpSetContentValid3SessionChange.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiffJson(jsonExpected, jsonDumped));
}

void test_contentsets::contentSetsSelectValid4SessionChange()
{
    m_testSystem.changeSession();
    m_testSystem.setComponent(dataLoggerEntityId, LoggerStaticTexts::s_currentContentSetsComponentName, QVariantList() << "TestSet4");

    QFile file(":/vein-dumps/dumpSetContentValid4SessionChange.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiffJson(jsonExpected, jsonDumped));
}

void test_contentsets::contentSetsSelectValidAllSessionChange()
{
    m_testSystem.changeSession();
    m_testSystem.setComponent(dataLoggerEntityId, LoggerStaticTexts::s_currentContentSetsComponentName, QVariantList() << "ZeraAll");

    QFile file(":/vein-dumps/dumpSetContentValidAllSessionChange.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = m_testSystem.dumpStorage();
    QVERIFY(TestLogHelpers::compareAndLogOnDiffJson(jsonExpected, jsonDumped));
}
