#include "test_db_command_interface_meta_registered.h"
#include "testcomponentinforeceiver.h"
#include <timemachineobject.h>
#include <QTest>

QTEST_MAIN(test_db_command_interface_meta_registered)

using namespace VeinLogger;

void test_db_command_interface_meta_registered::sendComponentAddQeued()
{
    DatabaseCommandInterface dbInterface;
    TestComponentInfoReceiver receiver;
    connect(&dbInterface, &DatabaseCommandInterface::sigAddLoggedValue,
            &receiver, &TestComponentInfoReceiver::onComponentAddReceived,
            Qt::QueuedConnection);

    DatabaseCommandInterface::ComponentInfo dummyInfo;

    emit dbInterface.sigAddLoggedValue("foo", QVector<int>(), dummyInfo);
    emit dbInterface.sigAddLoggedValue("foo", QVector<int>(), dummyInfo);
    TimeMachineObject::feedEventLoop();

    QCOMPARE(receiver.getInfosReceived().count(), 2);
}

void test_db_command_interface_meta_registered::sendSessionAddQueued()
{
    DatabaseCommandInterface dbInterface;
    TestComponentInfoReceiver receiver;
    connect(&dbInterface, &DatabaseCommandInterface::sigAddSession,
            &receiver, &TestComponentInfoReceiver::onSessionAddReceived,
            Qt::QueuedConnection);

    DatabaseCommandInterface::ComponentInfo dummyInfo;
    emit dbInterface.sigAddSession("foo", QList<DatabaseCommandInterface::ComponentInfo>() << dummyInfo);
    emit dbInterface.sigAddSession("foo", QList<DatabaseCommandInterface::ComponentInfo>() << dummyInfo << dummyInfo);
    TimeMachineObject::feedEventLoop();

    QCOMPARE(receiver.getInfosReceived().count(), 3);
}
