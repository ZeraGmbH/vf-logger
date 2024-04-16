#include "test_testlib_testloggersystem.h"
#include "testdumpreporter.h"
#include <QTest>

QTEST_MAIN(test_testlib_testloggersystem)

void test_testlib_testloggersystem::init()
{
    m_testSystem.setupServer();
}

void test_testlib_testloggersystem::cleanup()
{
    m_testSystem.cleanup();
}

void test_testlib_testloggersystem::systemSetupProperly()
{
    QFile file(":/dumpInitial.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QByteArray jsonDumped = m_testSystem.dumpStorage(QList<int>() << systemEntityId << dataLoggerEntityId << 10 << 11 << 12);

    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}
