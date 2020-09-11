#include "jsoncontextloader.h"

#include <QFile>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <exception>



JsonContextLoader::JsonContextLoader(QObject *parent) : QObject(parent),
    m_zeraContextPath(""),
    m_customerContextPath(""),
    m_lastError(error::NoError)
{

}

bool JsonContextLoader::init(const QString &p_zeraContextPath, const QString &p_customerContextPath)
{
    bool retVal=true;

    m_zeraContextPath = p_zeraContextPath;
    m_customerContextPath = p_customerContextPath;

    QFile zeraFile;
    QFile customerFile;

    zeraFile.setFileName(p_zeraContextPath);
    customerFile.setFileName(p_customerContextPath);

    if(!zeraFile.exists()){
        m_zeraContextPath="";
        retVal=false;
        m_lastError=error::FileDoesNotExist;
    }

    if(!customerFile.exists()){
        //m_customerContextPath="";
        //retVal=false;
        //m_lastError=error::FileDoesNotExist;
    }


    return retVal;
}

QMap<QString,QVector<QString>> JsonContextLoader::readContext(const QString &p_contextName)
{
    QMap<QString,QVector<QString>> retVal;
    try {
        if(hasContext(m_zeraContextPath,p_contextName)){
            retVal=readContextFromFile(m_zeraContextPath,p_contextName);
        }else if(hasContext(m_customerContextPath,p_contextName)){
            retVal=readContextFromFile(m_customerContextPath,p_contextName);
        }
    }  catch (error &e) {
        m_lastError=e;
    }
    return retVal;

}

bool JsonContextLoader::addContext(const QString &p_contextName, const QString &p_session, QMap<QString,QVector<QString>> p_entityComponentMap)
{
    bool retVal=true;
    try {
        QVector<QString> retVal;
        QFile file;
        if(m_customerContextPath.isEmpty()){
            throw error::FileDoesNotExist;
        }
        file.setFileName(m_customerContextPath);
        if(!file.open(QIODevice::ReadWrite)){
            throw error::CanNotOpenFile;
        }
        QByteArray fileContent=file.readAll();
        file.close();
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(fileContent, &err);
        if(err.error != QJsonParseError::NoError){
            // throw error::JsonParserError;
        }

        // Add new Context to Session
        QJsonObject rootObj;
        if(doc.isObject()){
            rootObj = doc.object();
        }
        QJsonObject sessionObj = rootObj.value(c_session).toObject();

        if(rootObj.value(c_session) == QJsonValue::Undefined){
            rootObj.insert(c_session,QJsonObject());
        }
        QJsonArray session;
        if(!sessionObj.value(p_session).isUndefined()){
            session = sessionObj.value(p_session).toArray();
        }
        if(!session.contains(QJsonValue(p_contextName))){
            session.append(QJsonValue(p_contextName));
        }
        sessionObj.insert(p_session,session);
        rootObj.insert(c_session,sessionObj);

        // Add Context
        QJsonObject contextObj = rootObj.value(c_context).toObject();
        if(rootObj.value(c_context) == QJsonValue::Undefined){
            rootObj.insert(c_context,QJsonObject());
        }
        QJsonArray context;
        QJsonObject element;
        for(QString tmpEnt : p_entityComponentMap.keys()){
            element.insert(c_entity,tmpEnt);
            element.insert(c_component,QJsonArray::fromStringList(p_entityComponentMap[tmpEnt].toList()));
            if(!context.contains(element)){
                context.append(element);
            }
        }

        contextObj.insert(p_contextName,context);
        rootObj.insert(c_context,contextObj);
        //write to object
        doc.setObject(rootObj);


        file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate);
        file.write(doc.toJson());
        file.close();


    }  catch (error &e) {
        retVal=false;
        m_lastError=e;

    }
    return retVal;
}

bool JsonContextLoader::removeContext(const QString &p_contextName)
{
    bool retVal=true;
    try {
        QFile file;
        if(m_customerContextPath.isEmpty()){
            throw error::FileDoesNotExist;
        }
        file.setFileName(m_customerContextPath);
        if(!file.open(QIODevice::ReadWrite)){
            throw error::CanNotOpenFile;
        }
        QByteArray fileContent=file.readAll();
        file.close();
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(fileContent, &err);
        if(err.error != QJsonParseError::NoError){
            throw error::JsonParserError;
        }
        if(!doc.isObject()){
            throw error::ObjectDoesNotExist;
        }
        // Remove Context From Session
        QJsonObject rootObj;
        if(doc.isObject()){
            rootObj = doc.object();
        }
        QJsonObject sessionObj = rootObj.value(c_session).toObject();

        if(rootObj.value(c_session) == QJsonValue::Undefined){
            throw error::ObjectDoesNotExist;
        }

        for(QString session : sessionObj.keys()){
            QJsonArray arr = sessionObj.value(session).toArray();
            QJsonArray newArr;
            for(QJsonArray::Iterator ele = arr.begin(); ele != arr.end(); ++ele){
                if(ele->toString() != p_contextName){
                    newArr.append(ele->toString());
                }
            }
            sessionObj.insert(session,newArr);
        }

        rootObj.insert(c_session,sessionObj);


        //Remove Context

        QJsonObject contextObj = rootObj.value(c_context).toObject();
        contextObj.remove(p_contextName);

        rootObj.insert(c_context,contextObj);

        doc.setObject(rootObj);

        file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate);
        file.write(doc.toJson());
        file.close();


    }  catch (error &e) {
        retVal=false;
        m_lastError=e;

    }
    return retVal;
}


QVector<QString> JsonContextLoader::contextList(const QString &p_session)
{
    QSet<QString> retVal;
    try{
        retVal.unite(zeraContextList(p_session).toList().toSet());
        retVal.unite(customerContextList(p_session).toList().toSet());
    }catch(error &e){
        m_lastError=e;
    }
    return retVal.values().toVector();
}

QVector<QString> JsonContextLoader::sessionList()
{
    QSet<QString> retVal;
    try{
        retVal.unite(zeraSessionList().toList().toSet());
        retVal.unite(customerSessionList().toList().toSet());
    }catch(error &e){
        m_lastError=e;
    }
    return retVal.values().toVector();
}

JsonContextLoader::error JsonContextLoader::readLasterror()
{
    error tmpError=m_lastError;
    m_lastError=error::NoError;
    return tmpError;
}


QVector<QString> JsonContextLoader::zeraContextList(const QString &p_session)
{
    QVector<QString> retVal;
    if(!m_zeraContextPath.isEmpty()){
        retVal = readContextListFromFile(m_zeraContextPath, p_session);
    }
    return retVal;
}

QVector<QString> JsonContextLoader::customerContextList(const QString &p_session)
{
    QVector<QString> retVal;
    if(!m_customerContextPath.isEmpty()){
        retVal = readContextListFromFile(m_customerContextPath, p_session);
    }
    return retVal;
}

QVector<QString> JsonContextLoader::zeraSessionList()
{
    QVector<QString> retVal;
    if(!m_zeraContextPath.isEmpty()){
        retVal = readSessionListFromFile(m_zeraContextPath);
    }
    return retVal;
}

QVector<QString> JsonContextLoader::customerSessionList()
{
    QVector<QString> retVal;
    if(!m_customerContextPath.isEmpty()){
        retVal = readSessionListFromFile(m_customerContextPath);
    }
    return retVal;
}

QVector<QString> JsonContextLoader::readSessionListFromFile(const QString &p_file)
{
    QVector<QString> retVal;
    QFile file;
    file.setFileName(p_file);
    if(!file.open(QIODevice::Unbuffered | QIODevice::ReadOnly)){
        throw error::CanNotOpenFile;
    }
    QByteArray fileContent=file.readAll();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(fileContent, &err);
    if(err.error == QJsonParseError::NoError){
        if(doc.isObject()){
            QJsonObject rootObj = doc.object();
            QJsonObject sessionObj = rootObj.value(c_session).toObject();
            retVal=sessionObj.keys().toVector();
        }
    }
    file.close();
    return retVal;
}

QVector<QString> JsonContextLoader::readContextListFromFile(const QString &p_file, const QString &p_session)
{
    QVector<QString> retVal;
    QFile file;
    file.setFileName(p_file);
    if(!file.open(QIODevice::Unbuffered | QIODevice::ReadOnly)){
        throw error::CanNotOpenFile;
    }
    QByteArray fileContent=file.readAll();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(fileContent, &err);
    if(err.error == QJsonParseError::NoError){
        if(doc.isObject()){
            QJsonObject rootObj = doc.object();
            QJsonValue sessionObj = rootObj.value(c_session);
            if(sessionObj == QJsonValue::Undefined){
                file.close();
                throw error::ObjectDoesNotExist;
            }
            QJsonValue  contextList = sessionObj.toObject().value(p_session);
            if(contextList == QJsonValue::Undefined){
            }else{
                for(QJsonValue tmpVal : contextList.toArray()){
                    retVal.append(tmpVal.toString());
                }
            }

        }
    }
    file.close();
    return retVal;
}

bool JsonContextLoader::hasContext(const QString &p_file, const QString &p_contextName)
{

    bool retVal=false;
    QFile file;
    file.setFileName(p_file);
    if(file.exists()){
        if(!file.open(QIODevice::Unbuffered | QIODevice::ReadOnly)){
            throw std::runtime_error("Can not open File");
        }
        QByteArray fileContent=file.readAll();
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(fileContent, &err);
        if(err.error == QJsonParseError::NoError){
            if(doc.isObject()){
                QJsonObject rootObj = doc.object();
                QJsonValue contextObj = rootObj.value(c_context);
                if(contextObj.isUndefined()){
                    file.close();
                    throw error::ObjectDoesNotExist;
                }
                QJsonValue  contextList = contextObj.toObject().value(p_contextName);
                if(contextList.isUndefined()){
                    retVal=false;
                }else{
                    retVal=true;
                }
            }
        }
        file.close();
    }
    return retVal;
}

QMap<QString,QVector<QString>> JsonContextLoader::readContextFromFile(const QString &p_file, const QString &p_context)
{
    QMap<QString,QVector<QString>> retVal;
    QFile file;
    file.setFileName(p_file);
    if(!file.open(QIODevice::Unbuffered | QIODevice::ReadOnly)){
        throw error::CanNotOpenFile;
    }
    QByteArray fileContent=file.readAll();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(fileContent, &err);
    if(err.error == QJsonParseError::NoError){
        if(doc.isObject()){
            QJsonObject rootObj = doc.object();
            QJsonValue contextObj = rootObj.value(c_context);
            if(contextObj == QJsonValue::Undefined){
                throw error::ObjectDoesNotExist;
            }
            QJsonValue contextList=contextObj.toObject().value(p_context);
            if(contextList == QJsonValue::Undefined){
                throw error::ObjectDoesNotExist;
            }
            for(QJsonValue tmpVal : contextList.toArray()){
                QJsonValue compList=tmpVal.toObject().value(c_component);
                for(QJsonValue comp : compList.toArray()){
                    retVal[tmpVal.toObject().value(c_entity).toString()].append(comp.toString());
                }

            }
        }
    }
    file.close();
    return retVal;
}
