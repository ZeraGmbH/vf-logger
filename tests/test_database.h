#ifndef TEST_DATABASE_H
#define TEST_DATABASE_H

#include "testloggersystem.h"
#include <QObject>

class test_database : public QObject
{
    Q_OBJECT
private slots:
    void cleanup();
    void displaySessionInfo_data();
    void displaySessionInfo();
private:
    std::unique_ptr<TestLoggerSystem> m_testSystem;
};

#endif // TEST_DATABASE_H
