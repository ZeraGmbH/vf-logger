#ifndef JSONCONTEXTLOADER_H
#define JSONCONTEXTLOADER_H

#include <QObject>

class JsonContextLoader : public QObject
{
    Q_OBJECT
public:

    enum class error{
        NoError,
        FileDoesNotExist,
        ObjectDoesNotExist,
        ObjectHasNoValue,
        CanNotOpenFile,
        JsonParserError
    };

    explicit JsonContextLoader(QObject *parent = nullptr);

    bool init(const QString &p_zeraContextPath, const QString &p_customerContextPath);

    QMap<QString,QVector<QString>>  readContext(const QString &p_contextName);
    bool addContext(const QString &p_contextName, const QString &p_session, QMap<QString,QVector<QString>>  p_entityComponentMap);
    bool removeContext(const QString &p_contextName);
    QVector<QString> contextList(const QString &p_session);
    QVector<QString> sessionList();
    error readLasterror();
private:
    QVector<QString> zeraContextList(const QString &p_session);
    QVector<QString> customerContextList(const QString &p_file);

    QVector<QString> zeraSessionList();
    QVector<QString> customerSessionList();

    QVector<QString> readSessionListFromFile(const QString &p_session);

    QVector<QString> readContextListFromFile(const QString &p_file,  const QString &p_session);
    bool hasContext(const QString &p_file,const QString &p_contextName);
    QMap<QString,QVector<QString>>  readContextFromFile(const QString &p_file,  const QString &p_context);

private:
    QString m_zeraContextPath;
    QString m_customerContextPath;
    error m_lastError;

    const QString c_session = QLatin1String("Sessions");
    const QString c_context = QLatin1String("Context");
    const QString c_entity = QLatin1String("EntityId");
    const QString c_component = QLatin1String("Components");

signals:

};

#endif // JSONCONTEXTLOADER_H
