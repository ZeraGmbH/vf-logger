#include "test_entities_stored_always.h"
#include "testloghelpers.h"
#include "testloggerdb.h"
#include <timemachineobject.h>
#include <QSignalSpy>
#include <QTest>

QTEST_MAIN(test_entities_stored_always)

using namespace VeinLogger;

static constexpr int entityIdStoredAlways = 37;
static constexpr int entityId = 42;

void test_entities_stored_always::cleanup()
{
    m_testSystem.cleanup();
}

void test_entities_stored_always::recordEntitiesStoredAlwaysOnly()
{
    m_testSystem.setupServer(3, 2, QList<int>() << 10 << 11);
    m_testSystem.loadDatabase();
    m_testSystem.setComponentValues(1);

    m_testSystem.startLogging();
    m_testSystem.setComponentValues(2);

    QFile file(":/recording-dumps/dumpRecordAlwaysTwoEntities.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = TestLoggerDB::getCurrentInstance()->getJsonDumpedComponentStored();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_entities_stored_always::recordEntitiesStoredAlwaysAndOthers()
{
    m_testSystem.setupServer(3, 3, QList<int>() << 10 << 11);
    m_testSystem.loadDatabase();

    // To simplify we use "LoggedComponents" not "currentContentSets" (LoggerStaticTexts::s_currentContentSetsComponentName)
    // BTW: This is the only place, we check that this still works
    QVariantMap onDemandLoggedComponents;
    onDemandLoggedComponents["12"] = QStringList() << "ComponentName1" << "ComponentName3";
    m_testSystem.setComponent(dataLoggerEntityId, "LoggedComponents", onDemandLoggedComponents);
    m_testSystem.setComponentValues(1);

    m_testSystem.startLogging();
    m_testSystem.setComponentValues(2);

    QFile file(":/recording-dumps/dumpRecordAlwaysTwoSetOneEntities.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = TestLoggerDB::getCurrentInstance()->getJsonDumpedComponentStored();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_entities_stored_always::recordEntitiesStoredAlwaysTwoRecordings()
{
    m_testSystem.setupServer(3, 3, QList<int>() << 10); // 10 logged always
    m_testSystem.loadDatabase();

    QVariantMap onDemandLoggedComponents; // + 12 logged
    onDemandLoggedComponents["12"] = QStringList() << "ComponentName1" << "ComponentName3";
    m_testSystem.setComponent(dataLoggerEntityId, "LoggedComponents", onDemandLoggedComponents);
    m_testSystem.setComponentValues(1);

    m_testSystem.startLogging();
    m_testSystem.setComponentValues(2);

    m_testSystem.stopLogging();

    m_testSystem.setComponent(dataLoggerEntityId, "LoggedComponents", QVariantMap()); // remove 12
    // OnLoggerStartValues is overwritten so make it detectable
    m_testSystem.setComponent(10, "ComponentName1", "This is start data after restart");

    m_testSystem.setNextValueWriteCount(10000);
    m_testSystem.startLogging();
    m_testSystem.setComponentValues(1);

    QFile file(":/recording-dumps/dumpRecordAlwaysStartStopStart.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();
    QByteArray jsonDumped = TestLoggerDB::getCurrentInstance()->getJsonDumpedComponentStored();
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}
