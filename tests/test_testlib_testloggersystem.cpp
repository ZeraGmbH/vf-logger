#include "test_testlib_testloggersystem.h"
#include "testloghelpers.h"
#include <QTest>

QTEST_MAIN(test_testlib_testloggersystem)

void test_testlib_testloggersystem::init()
{
    m_testSystem.setupServer(3, 3);
}

void test_testlib_testloggersystem::cleanup()
{
    m_testSystem.cleanup();
}

void test_testlib_testloggersystem::systemSetupProperly()
{
    QFile file(":/vein-dumps/dumpInitial.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QByteArray jsonDumped = m_testSystem.dumpStorage(QList<int>() << systemEntityId << dataLoggerEntityId << m_testSystem.getComponentsCreated().keys());

    QVERIFY(TestLogHelpers::compareAndLogOnDiffJson(jsonExpected, jsonDumped));
}
