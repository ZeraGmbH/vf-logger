#include "databasefilepathchecker.h"
#include <QFileInfo>
#include <QDir>

QString DatabaseFilePathChecker::createAndCheckDir(const QString &filePath)
{
    QFileInfo fInfo(filePath);
    QString errMsg;
    if(!fInfo.isRelative()) {
        // try to create path
        if(!fInfo.absoluteDir().exists()) {
            QDir dir;
            dir.mkpath(fInfo.absoluteDir().path());
        }
        if(fInfo.absoluteDir().exists()) {
            if(fInfo.exists() && !fInfo.isFile())
                errMsg = QString("Path is not a valid file location: %1").arg(filePath);
        }
        else
            errMsg = QString("Parent directory for path does not exist: %1").arg(filePath);
    }
    else
        errMsg = QString("Relative paths are not accepted: %1").arg(filePath);
    return errMsg;
}
