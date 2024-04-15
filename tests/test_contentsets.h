#ifndef TEST_CONTENTSETS_H
#define TEST_CONTENTSETS_H

#include "vsc_scriptsystem.h"
#include "veinqml.h"
#include <zeradblogger.h>
#include <testveinserver.h>
#include <QObject>
#include <memory>

class test_contentsets : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void init();
    void cleanup();
    void loggerSetupProperly();
    void contentSetsSelectValid();
    void contentSetsSelectInvalid();
    void contentSetsSelectValidList();
    void contentSetsSelectValidListTwo();
    void contentSetsSelectValidListTwoSame();
    void contentSetsSelectValidListAll();
    void contentSetsSelectValidListSequence();
private:
    void setupServer();
    std::unique_ptr<TestVeinServer> m_server;
    VeinEvent::StorageSystem* m_storage;
    std::unique_ptr<VeinScript::ScriptSystem> m_scriptSystem;
    std::unique_ptr<VeinApiQml::VeinQml> m_qmlSystem;
    std::unique_ptr<ZeraDBLogger> m_dataLoggerSystem;
};

#endif // TEST_CONTENTSETS_H
