#include "databasefilewatcher.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStorageInfo>

DatabaseFileWatcher::DatabaseFileWatcher()
{
}

void DatabaseFileWatcher::startWatch(const QString filePath)
{
    m_databaseFilePath = filePath;
    // * To avoid fire storm on logging we watch file's dir
    // * For removable devices: mount-point's parent dir
    QFileInfo fileInfo(m_databaseFilePath);
    QStorageInfo storageInfo(fileInfo.absolutePath());
    QStringList watchedPaths;
    watchedPaths.append(fileInfo.absolutePath());
    if(!storageInfo.isRoot()) {
        QDir tmpDir(storageInfo.rootPath());
        tmpDir.cdUp();
        if(!tmpDir.isRoot())
            watchedPaths.append(tmpDir.path());
    }
    qInfo("Database logger watching path(s): %s", qPrintable(watchedPaths.join(QStringLiteral(" + "))));
    QStringList unWatchedPaths = m_deleteWatcher.addPaths(watchedPaths);
    if(m_deleteWatcher.directories().count())
        QObject::connect(&m_deleteWatcher, &QFileSystemWatcher::directoryChanged, this, &DatabaseFileWatcher::checkDatabaseStillValid);
    if(unWatchedPaths.count())
        qWarning("Unwatched paths: %s", qPrintable(unWatchedPaths.join(QStringLiteral(" + "))));

}

void DatabaseFileWatcher::checkDatabaseStillValid()
{
    QFile dbFile(m_databaseFilePath);
    if(!dbFile.exists())
        emit sigFileError();

}
