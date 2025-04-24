#ifndef TEST_CONTENTSETS_H
#define TEST_CONTENTSETS_H

#include "testloggersystem.h"

class test_contentsets : public QObject
{
    Q_OBJECT
private slots:
    void init();
    void cleanup();

    void contentSetsJsonEnviornmentSetTwice();

    void contentSetsSelectValidStringToBeFixed();
    void contentSetsSelectInvalid();
    void contentSetsSelectValid();
    void contentSetsSelectValidTwo();
    void contentSetsSelectValidTwoSame();
    void contentSetsSelectValidAll();
    void contentSetsSelectValidSequence();

    void contentSetsSelectValid3SessionChange();
    void contentSetsSelectValid4SessionChange();
    void contentSetsSelectValidAllSessionChange();

private:
    TestLoggerSystem m_testSystem;
};

#endif // TEST_CONTENTSETS_H
