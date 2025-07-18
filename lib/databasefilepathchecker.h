#ifndef DATABASEFILEPATHCHECKER_H
#define DATABASEFILEPATHCHECKER_H

#include <QString>

class DatabaseFilePathChecker
{
public:
    static QString createAndCheckDir(const QString &filePath);
};

#endif // DATABASEFILEPATHCHECKER_H
