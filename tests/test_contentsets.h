#ifndef TEST_CONTENTSETS_H
#define TEST_CONTENTSETS_H

#include "testloggersystem.h"

class test_contentsets : public QObject
{
    Q_OBJECT
private slots:
    void init();
    void cleanup();

    void contentSetsSelectValid();
    void contentSetsSelectInvalid();
    void contentSetsSelectValidList();
    void contentSetsSelectValidListTwo();
    void contentSetsSelectValidListTwoSame();
    void contentSetsSelectValidListAll();
    void contentSetsSelectValidListSequence();
private:
    TestLoggerSystem m_testSystem;
};

#endif // TEST_CONTENTSETS_H
