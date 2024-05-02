#ifndef TEST_DB_COMMAND_INTERFACE_META_REGISTERED_H
#define TEST_DB_COMMAND_INTERFACE_META_REGISTERED_H

#include <QObject>

class test_db_command_interface_meta_registered : public QObject
{
    Q_OBJECT
private slots:
    void sendComponentAddQeued();
    void sendSessionAddQueued();
};

#endif // TEST_DB_COMMAND_INTERFACE_META_REGISTERED_H
