#ifndef TEST_ENTITIES_STORED_ALWAYS_H
#define TEST_ENTITIES_STORED_ALWAYS_H

#include "testloggersystem.h"

class test_entities_stored_always : public QObject
{
    Q_OBJECT
private slots:
    void cleanup();
    void recordEntitiesStoredAlwaysOnly();
    void recordEntitiesStoredAlwaysAndOthers();
    void recordEntitiesStoredAlwaysTwoRecordings();
private:
    TestLoggerSystem m_testSystem;
};

#endif // TEST_ENTITIES_STORED_ALWAYS_H
