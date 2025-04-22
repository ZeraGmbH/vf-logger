#include "test_database.h"
#include "testloghelpers.h"
#include <QTest>

Q_DECLARE_METATYPE(TestLoggerSystem::DbType)
QTEST_MAIN(test_database)

void test_database::initTestCase_data()
{
    QTest::addColumn<TestLoggerSystem::DbType>("DbType");
    QTest::newRow("Mock Databse") << TestLoggerSystem::DbType::MOCK;
    QTest::newRow("SQLite Databse") << TestLoggerSystem::DbType::SQLITE;
}

void test_database::init()
{
    QFETCH_GLOBAL(TestLoggerSystem::DbType, DbType);
    m_testSystem = std::make_unique<TestLoggerSystem>(DbType);
}

void test_database::cleanup()
{
    m_testSystem->cleanup();
    m_testSystem.reset();
}

void test_database::displaySessionInfo()
{
    QString sessionName = "Session1";
    QString transactionName = "Transaction1";

    m_testSystem->setupServer();
    m_testSystem->loadDatabase();
    m_testSystem->setComponent(dataLoggerEntityId, "sessionName", sessionName);
    m_testSystem->setComponent(dataLoggerEntityId, "guiContext", "ZeraGuiActualValues");
    m_testSystem->setComponent(dataLoggerEntityId, "currentContentSets", QVariantList() << "ZeraAll");
    m_testSystem->startLogging("Session1", transactionName);
    m_testSystem->stopLogging();

    QFile file(":/sqlite-inserts/dumpSessionInfos.json");
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray jsonExpected = file.readAll();

    QJsonObject jsonObj = m_testSystem->displaySessionsInfos(sessionName);
    //Remove 'Time' information as it indicates the actual time when logging was done. So it will never match with 'Time' from jsonExpected.
    QJsonObject temp = jsonObj.value(transactionName).toObject();
    temp.insert("Time", QString());
    jsonObj.insert(transactionName, temp);

    QJsonDocument jsonDoc(jsonObj);
    QString jsonDumped = jsonDoc.toJson(QJsonDocument::Indented);
    QVERIFY(TestLogHelpers::compareAndLogOnDiff(jsonExpected, jsonDumped));
}
