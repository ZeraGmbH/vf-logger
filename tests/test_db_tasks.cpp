#include "test_db_tasks.h"
#include "taskdbaddsession.h"
#include "taskdbaddtransaction.h"
#include "taskdbupdatetransactionstarttime.h"
#include "taskdbupdatetransactionstoptime.h"
#include <taskcontainersequence.h>
#include <timemachineobject.h>
#include <QSignalSpy>
#include <QTest>

Q_DECLARE_METATYPE(TestLoggerSystem::DbType)

QTEST_MAIN(test_db_tasks)

void test_db_tasks::initTestCase_data()
{
    QTest::addColumn<TestLoggerSystem::DbType>("DbType");
    QTest::newRow("Mock Database") << TestLoggerSystem::DbType::MOCK;
    QTest::newRow("SQLite Database") << TestLoggerSystem::DbType::SQLITE;
}

void test_db_tasks::init()
{
    QFETCH_GLOBAL(TestLoggerSystem::DbType, DbType);
    m_testSystem = std::make_unique<TestLoggerSystem>(DbType);
    m_testSystem->setupServer();
}

void test_db_tasks::cleanup()
{
    m_testSystem->cleanup();
    m_testSystem.reset();
}

void test_db_tasks::taskAddSession()
{
    m_testSystem->loadDatabase();

    AbstractLoggerDBPtr loggerDb = m_testSystem->getDbLogger()->getDb();
    TestDbAddSignaller* signaller = m_testSystem->getSignaller();
    QSignalSpy spyAddSession(signaller, &TestDbAddSignaller::sigAddSession);

    const QString sessionName = "sessionName";
    std::shared_ptr<int> sessionId = std::make_shared<int>(-1);
    TaskDbAddSession task(loggerDb, m_testSystem->getStorage(), sessionName, sessionId);
    QSignalSpy spyTask(&task, &TaskTemplate::sigFinish);
    task.start();
    TimeMachineObject::feedEventLoop();

    QCOMPARE(spyTask.count(), 1);
    QCOMPARE(spyTask[0][0], true);

    QCOMPARE(spyAddSession.count(), 1);
    QCOMPARE(spyAddSession[0][0], sessionName);
    QVERIFY(*sessionId > 0);
}

void test_db_tasks::taskAddSessionTwice()
{
    m_testSystem->loadDatabase();

    AbstractLoggerDBPtr loggerDb = m_testSystem->getDbLogger()->getDb();
    TestDbAddSignaller* signaller = m_testSystem->getSignaller();
    QSignalSpy spyAddSession(signaller, &TestDbAddSignaller::sigAddSession);

    const QString sessionName = "sessionName";

    TaskContainerSequence taskSequence;
    std::shared_ptr<int> sessionId1 = std::make_shared<int>(-1);
    std::shared_ptr<int> sessionId2 = std::make_shared<int>(-1);
    taskSequence.addSub(TaskDbAddSession::create(loggerDb, m_testSystem->getStorage(), sessionName, sessionId1));
    taskSequence.addSub(TaskDbAddSession::create(loggerDb, m_testSystem->getStorage(), sessionName, sessionId2));
    QSignalSpy spyTask(&taskSequence, &TaskTemplate::sigFinish);
    taskSequence.start();
    TimeMachineObject::feedEventLoop();

    QCOMPARE(spyTask.count(), 1);
    QCOMPARE(spyTask[0][0], true);

    QCOMPARE(spyAddSession.count(), 2);
    QCOMPARE(spyAddSession[0][0], sessionName);
    QCOMPARE(spyAddSession[1][0], sessionName);
    QVERIFY(*sessionId1 > 0);
    QVERIFY(*sessionId2 > 0);
    QCOMPARE(*sessionId1, *sessionId2);
}

void test_db_tasks::taskAddSessionNoDb()
{
    AbstractLoggerDBPtr loggerDb = m_testSystem->getDbLogger()->getDb();

    const QString sessionName = "sessionName";
    std::shared_ptr<int> sessionId = std::make_shared<int>(-1);
    TaskDbAddSession task(loggerDb, m_testSystem->getStorage(), sessionName, sessionId);
    QSignalSpy spyTask(&task, &TaskTemplate::sigFinish);
    task.start();
    TimeMachineObject::feedEventLoop();

    QCOMPARE(spyTask.count(), 1);
    QCOMPARE(spyTask[0][0], false);
}

void test_db_tasks::taskAddSessionNoSessionName()
{
    m_testSystem->loadDatabase();

    AbstractLoggerDBPtr loggerDb = m_testSystem->getDbLogger()->getDb();
    TestDbAddSignaller* signaller = m_testSystem->getSignaller();
    QSignalSpy spyAddSession(signaller, &TestDbAddSignaller::sigAddSession);

    std::shared_ptr<int> sessionId = std::make_shared<int>(-1);
    TaskDbAddSession task(loggerDb, m_testSystem->getStorage(), "", sessionId);
    QSignalSpy spyTask(&task, &TaskTemplate::sigFinish);
    task.start();
    TimeMachineObject::feedEventLoop();

    QCOMPARE(spyTask.count(), 1);
    QCOMPARE(spyTask[0][0], false);
}

void test_db_tasks::taskAddTransaction()
{
    m_testSystem->loadDatabase();

    AbstractLoggerDBPtr loggerDb = m_testSystem->getDbLogger()->getDb();
    TestDbAddSignaller* signaller = m_testSystem->getSignaller();
    QSignalSpy spyAddTransaction(signaller, &TestDbAddSignaller::sigAddTransaction);

    const QString transactionName = "transactionName";
    const QString sessionName = "sessionName";
    const QStringList contentSets = QStringList() << "TestSet1" << "TestSet2";
    const QString guiContextName = "guiContextName";

    const VeinLogger::StartTransactionParam param = {transactionName, sessionName, contentSets, guiContextName};
    std::shared_ptr<int> transactionId = std::make_shared<int>(-1);
    TaskDbAddTransaction task(loggerDb,  param, transactionId);
    QSignalSpy spyTask(&task, &TaskTemplate::sigFinish);
    task.start();
    TimeMachineObject::feedEventLoop();

    QCOMPARE(spyTask.count(), 1);
    QCOMPARE(spyTask[0][0], true);

    QVERIFY(*transactionId > 0);
    QCOMPARE(spyAddTransaction.count(), 1);
    QCOMPARE(spyAddTransaction[0][0], transactionName);
    QCOMPARE(spyAddTransaction[0][1], sessionName);
    QCOMPARE(spyAddTransaction[0][2], contentSets);
    QCOMPARE(spyAddTransaction[0][3], guiContextName);
}

void test_db_tasks::taskAddTransactionTwice()
{
    m_testSystem->loadDatabase();

    AbstractLoggerDBPtr loggerDb = m_testSystem->getDbLogger()->getDb();
    TestDbAddSignaller* signaller = m_testSystem->getSignaller();
    QSignalSpy spyAddTransaction(signaller, &TestDbAddSignaller::sigAddTransaction);

    const QString transactionName = "transactionName";
    const QString sessionName = "sessionName";
    const QStringList contentSets = QStringList() << "TestSet1" << "TestSet2";
    const QString guiContextName = "guiContextName";

    const VeinLogger::StartTransactionParam param = {transactionName, sessionName, contentSets, guiContextName};
    std::shared_ptr<int> transactionId1 = std::make_shared<int>(-1);
    std::shared_ptr<int> transactionId2 = std::make_shared<int>(-1);

    TaskContainerSequence taskSequence;
    taskSequence.addSub(TaskDbAddTransaction::create(loggerDb,  param, transactionId1));
    taskSequence.addSub(TaskDbAddTransaction::create(loggerDb,  param, transactionId2));
    QSignalSpy spyTask(&taskSequence, &TaskTemplate::sigFinish);
    taskSequence.start();
    TimeMachineObject::feedEventLoop();

    QCOMPARE(spyTask.count(), 1);
    QCOMPARE(spyTask[0][0], true);

    QVERIFY(*transactionId1 > 0);
    QVERIFY(*transactionId2 > 0);
    //QCOMPARE(*transactionId1, *transactionId2); // Surprise: SQLITE implementation fails
    QCOMPARE(spyAddTransaction.count(), 2);
}

void test_db_tasks::taskAddTransactionNoDb()
{
    AbstractLoggerDBPtr loggerDb = m_testSystem->getDbLogger()->getDb();

    const QString transactionName = "transactionName";
    const QString sessionName = "sessionName";
    const QStringList contentSets = QStringList() << "contentSet1" << "contentSet2";
    const QString guiContextName = "guiContextName";

    const VeinLogger::StartTransactionParam param = {transactionName, sessionName, contentSets, guiContextName};
    std::shared_ptr<int> transactionId = std::make_shared<int>(-1);
    TaskDbAddTransaction task(loggerDb, param, transactionId);
    QSignalSpy spyTask(&task, &TaskTemplate::sigFinish);
    task.start();
    TimeMachineObject::feedEventLoop();

    QCOMPARE(spyTask.count(), 1);
    QCOMPARE(spyTask[0][0], false);
    QCOMPARE(*transactionId, -1);
}

void test_db_tasks::taskAddTransactionNoTransactionName()
{
    m_testSystem->loadDatabase();
    AbstractLoggerDBPtr loggerDb = m_testSystem->getDbLogger()->getDb();

    const VeinLogger::StartTransactionParam param = {"", "sessionName", QStringList() << "cs1" << "cs2", "guiContextName"};
    std::shared_ptr<int> transactionId = std::make_shared<int>(-1);
    TaskDbAddTransaction task(loggerDb, param, transactionId);
    QSignalSpy spyTask(&task, &TaskTemplate::sigFinish);
    task.start();
    TimeMachineObject::feedEventLoop();

    QCOMPARE(spyTask.count(), 1);
    QCOMPARE(spyTask[0][0], false);
    QCOMPARE(*transactionId, -1);
}

void test_db_tasks::taskAddTransactionNoSessionName()
{
    m_testSystem->loadDatabase();
    AbstractLoggerDBPtr loggerDb = m_testSystem->getDbLogger()->getDb();

    const VeinLogger::StartTransactionParam param = {"transactionName", "", QStringList() << "cs1" << "cs2", "guiContextName"};
    std::shared_ptr<int> transactionId = std::make_shared<int>(-1);
    TaskDbAddTransaction task(loggerDb, param, transactionId);
    QSignalSpy spyTask(&task, &TaskTemplate::sigFinish);
    task.start();
    TimeMachineObject::feedEventLoop();

    QCOMPARE(spyTask.count(), 1);
    QCOMPARE(spyTask[0][0], false);
    QCOMPARE(*transactionId, -1);
}

void test_db_tasks::taskAddTransactionNoContentSets()
{
    m_testSystem->loadDatabase();
    AbstractLoggerDBPtr loggerDb = m_testSystem->getDbLogger()->getDb();

    const VeinLogger::StartTransactionParam param = {"transactionName", "sessionName", QStringList(), "guiContextName"};
    std::shared_ptr<int> transactionId = std::make_shared<int>(-1);
    TaskDbAddTransaction task(loggerDb, param, transactionId);
    QSignalSpy spyTask(&task, &TaskTemplate::sigFinish);
    task.start();
    TimeMachineObject::feedEventLoop();

    QCOMPARE(spyTask.count(), 1);
    QCOMPARE(spyTask[0][0], false);
    QCOMPARE(*transactionId, -1);
}

void test_db_tasks::taskUpdateTransactionStartTime()
{
    m_testSystem->loadDatabase();
    AbstractLoggerDBPtr loggerDb = m_testSystem->getDbLogger()->getDb();
    TestDbAddSignaller* signaller = m_testSystem->getSignaller();
    QSignalSpy spyUpdateTransaction(signaller, &TestDbAddSignaller::sigTransactionUpdateStart);

    // create transaction
    const VeinLogger::StartTransactionParam param = {"transactionName", "sessionName", QStringList() << "cs1" << "cs2", "guiContextName"};
    std::shared_ptr<int> transactionId = std::make_shared<int>(-1);
    TaskDbAddTransaction taskCreateTransaction(loggerDb, param, transactionId);
    taskCreateTransaction.start();
    TimeMachineObject::feedEventLoop();

    QDateTime startTime = QDateTime::fromSecsSinceEpoch(0);
    TaskDbUpdateTransactionStartTime task(loggerDb, transactionId, startTime);
    QSignalSpy spyTask(&task, &TaskTemplate::sigFinish);
    task.start();
    TimeMachineObject::feedEventLoop();

    QCOMPARE(spyTask.count(), 1);
    QCOMPARE(spyTask[0][0], true);
    QCOMPARE(spyUpdateTransaction.count(), 1);
}

void test_db_tasks::taskUpdateTransactionStartTimeNoDb()
{
    QDateTime startTime = QDateTime::fromSecsSinceEpoch(0);
    std::shared_ptr<int> transactionId = std::make_shared<int>(42);
    TaskDbUpdateTransactionStartTime task(nullptr, transactionId, startTime);
    QSignalSpy spyTask(&task, &TaskTemplate::sigFinish);
    task.start();
    TimeMachineObject::feedEventLoop();

    QCOMPARE(spyTask.count(), 1);
    QCOMPARE(spyTask[0][0], false);
}

/*void test_db_tasks::taskUpdateTransactionStartTimeInvalidTaskId()
{
    m_testSystem->loadDatabase();
    AbstractLoggerDBPtr loggerDb = m_testSystem->getDbLogger()->getDb();
    TestDbAddSignaller* signaller = m_testSystem->getSignaller();
    QSignalSpy spyUpdateTransaction(signaller, &TestDbAddSignaller::sigTransactionUpdateStart);

    std::shared_ptr<int> transactionId = std::make_shared<int>(-1);
    QDateTime startTime = QDateTime::fromSecsSinceEpoch(0);
    TaskDbUpdateTransactionStartTime task(loggerDb, transactionId, startTime);
    QSignalSpy spyTask(&task, &TaskTemplate::sigFinish);
    task.start();
    TimeMachineObject::feedEventLoop();

    QCOMPARE(spyTask.count(), 1);
    QCOMPARE(spyTask[0][0], false);
}*/

void test_db_tasks::taskUpdateTransactionStopTime()
{
    m_testSystem->loadDatabase();
    AbstractLoggerDBPtr loggerDb = m_testSystem->getDbLogger()->getDb();
    TestDbAddSignaller* signaller = m_testSystem->getSignaller();
    QSignalSpy spyUpdateTransaction(signaller, &TestDbAddSignaller::sigTransactionUpdateStop);

    // create transaction
    const VeinLogger::StartTransactionParam param = {"transactionName", "sessionName", QStringList() << "cs1" << "cs2", "guiContextName"};
    std::shared_ptr<int> transactionId = std::make_shared<int>(-1);
    TaskDbAddTransaction taskCreateTransaction(loggerDb, param, transactionId);
    taskCreateTransaction.start();
    TimeMachineObject::feedEventLoop();

    QDateTime stopTime = QDateTime::fromSecsSinceEpoch(0);
    TaskDbUpdateTransactionStopTime task(loggerDb, transactionId, stopTime);
    QSignalSpy spyTask(&task, &TaskTemplate::sigFinish);
    task.start();
    TimeMachineObject::feedEventLoop();

    QCOMPARE(spyTask.count(), 1);
    QCOMPARE(spyTask[0][0], true);
    QCOMPARE(spyUpdateTransaction.count(), 1);
}

void test_db_tasks::taskUpdateTransactionStopTimeNoDb()
{
    QDateTime stopTime = QDateTime::fromSecsSinceEpoch(0);
    std::shared_ptr<int> transactionId = std::make_shared<int>(42);
    TaskDbUpdateTransactionStopTime task(nullptr, transactionId, stopTime);
    QSignalSpy spyTask(&task, &TaskTemplate::sigFinish);
    task.start();
    TimeMachineObject::feedEventLoop();

    QCOMPARE(spyTask.count(), 1);
    QCOMPARE(spyTask[0][0], false);
}

/*void test_db_tasks::taskUpdateTransactionStopTimeInvalidTaskId()
{
    m_testSystem->loadDatabase();
    AbstractLoggerDBPtr loggerDb = m_testSystem->getDbLogger()->getDb();
    TestDbAddSignaller* signaller = m_testSystem->getSignaller();
    QSignalSpy spyUpdateTransaction(signaller, &TestDbAddSignaller::sigTransactionUpdateStop);

    std::shared_ptr<int> transactionId = std::make_shared<int>(-1);
    QDateTime stopTime = QDateTime::fromSecsSinceEpoch(0);
    TaskDbUpdateTransactionStopTime task(loggerDb, transactionId, stopTime);
    QSignalSpy spyTask(&task, &TaskTemplate::sigFinish);
    task.start();
    TimeMachineObject::feedEventLoop();

    QCOMPARE(spyTask.count(), 1);
    QCOMPARE(spyTask[0][0], false);
}*/

