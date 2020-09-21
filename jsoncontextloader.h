#ifndef JSONCONTEXTLOADER_H
#define JSONCONTEXTLOADER_H

#include <QObject>




/**
 * @brief The JsonContextLoader class
 *
 * Reads json files of the following format.
 *
 * @code
 * {
 *   "Context": {
 *      "%{contextName}": [
            {
                "EntityId": "${id}",
                "Components": [
                    ${componentName},
                    ...
                ]
            },
 *   },
 *  "Sessions": {
 *      "${sessionName}": [
 *          ${contextName},
 *          ...
 *      ]
 *   }
 * }
 * @endcode
 *
 * To files are needed. One for defualt configuration.
 * This file is not editable (zeraContextPath).
 * And one editable file (customerContextPath).
 * The ouptut of this class merges the results of both files.
 *
 * zeraContextPath will always be priortised in case a a context is available in
 * both files.
 */
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
    /**
     * @brief init
     * @param p_zeraContextPath
     * @param p_customerContextPath
     * @return true on success
     */
    bool init(const QString &p_zeraContextPath, const QString &p_customerContextPath);
    /**
     * @brief readContext
     * @param p_contextName: context to read (<contextName>)
     * @return QMap<<QVector<componentName>>
     */
    QMap<QString,QVector<QString>>  readContext(const QString &p_contextName);
    /**
     * @brief addContext
     * @param p_contextName
     * @param p_session
     * @param p_entityComponentMap
     * @return true on success
     *
     * Add new context to custmerContextPath file.
     */
    bool addContext(const QString &p_contextName, const QString &p_session, QMap<QString,QVector<QString>>  p_entityComponentMap);
    /**
     * @brief removeContext
     * @param p_contextName
     * @return true on success
     *
     * Remove context from customerContextPath file.
     *
     * @ref error
     */
    bool removeContext(const QString &p_contextName);
    /**
     * @brief contextList
     * @param p_session
     * @return List with available contexts in the give session
     */
    QVector<QString> contextList(const QString &p_session);
    /**
     * @brief sessionList
     * @return List of Available sessions
     */
    QVector<QString> sessionList();
    /**
     * @brief readLasterror
     * @return returns the last occured error.
     *
     *
     */
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
