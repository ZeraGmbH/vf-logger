#include "test_contentsets.h"
#include "testdumpreporter.h"
#include <timemachineobject.h>
#include <QBuffer>
#include <QTest>

QTEST_MAIN(test_contentsets)

void test_contentsets::init()
{
    setupServer();
}

void test_contentsets::checkEntities()
{
    QFile file(":/dumpInitial.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QByteArray jsonDumped;
    QBuffer buff(&jsonDumped);
    m_storage->dumpToFile(&buff, QList<int>());

    QVERIFY(TestDumpReporter::compareAndLogOnDiff(jsonExpected, jsonDumped));
}

void test_contentsets::setupServer()
{
    m_server = std::make_unique<TestVeinServer>();
    m_storage = m_server->getStorage();

    for(int entityId = 10; entityId<13; entityId++) {
        QString entityName = QString("EntityName%1").arg(entityId);
        m_server->addEntity(entityId, entityName);
        for(int component=1; component<=3; component++) {
            QString componentName = QString("ComponentName%1").arg(component);
            m_server->addComponent(entityId, componentName, component, false);
        }
    }

    TimeMachineObject::feedEventLoop();
}
