#ifndef TEST_TESTDATABASE_H
#define TEST_TESTDATABASE_H

#include "vsc_scriptsystem.h"
#include "veinqml.h"
#include <zeradblogger.h>
#include <testveinserver.h>
#include <QObject>
#include <memory>

class test_testdatabase : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void init();
    void cleanup();

    void openDatabaseErrorEarly();
    void openDatabaseErrorLate();
    void openDatabaseOk();
    void setSessionNotExistentInDb();
private:
    void setupServer();
    void waitForDbThread();
    std::unique_ptr<TestVeinServer> m_server;
    VeinEvent::StorageSystem* m_storage;
    std::unique_ptr<VeinScript::ScriptSystem> m_scriptSystem;
    std::unique_ptr<VeinApiQml::VeinQml> m_qmlSystem;
    std::unique_ptr<ZeraDBLogger> m_dataLoggerSystem;
};

#endif // TEST_TESTDATABASE_H
