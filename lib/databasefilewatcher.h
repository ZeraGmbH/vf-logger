#ifndef DATABASEFILEWATCHER_H
#define DATABASEFILEWATCHER_H

#include <QObject>
#include <QFileSystemWatcher>

class DatabaseFileWatcher : public QObject
{
    Q_OBJECT
public:
    DatabaseFileWatcher();
    void startWatch(const QString filePath);
signals:
    void sigFileError();

private slots:
    void checkDatabaseStillValid();
private:
    QString m_databaseFilePath;
    QFileSystemWatcher m_deleteWatcher;
};

#endif // DATABASEFILEWATCHER_H
