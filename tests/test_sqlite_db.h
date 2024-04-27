#ifndef TEST_SQLITE_DB_H
#define TEST_SQLITE_DB_H

#include "testloggersystem.h"

class test_sqlite_db : public QObject
{
    Q_OBJECT
public:
    test_sqlite_db();
private slots:
    void cleanup();
    void createSessionInsertsEntityComponents();
    void createSessionWithCustomerDataAlreadyCreated();
private:
    void loadDatabase();

    TestLoggerSystem m_testSystem;
};

#endif // TEST_SQLITE_DB_H
