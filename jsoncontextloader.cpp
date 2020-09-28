#include "jsoncontextloader.h"

#include <QFile>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <exception>



JsonContentSetLoader::JsonContentSetLoader(QObject *parent) : QObject(parent),
    m_zeraContentSetPath(""),
    m_customerContentSetPath(""),
    m_lastError(error::NoError)
{

}

bool JsonContentSetLoader::init(const QString &p_zeraContentSetPath, const QString &p_customerContentSetPath)
{
    bool retVal=true;

    m_zeraContentSetPath = p_zeraContentSetPath;
    m_customerContentSetPath = p_customerContentSetPath;

    QFile zeraFile;
    QFile customerFile;

    zeraFile.setFileName(p_zeraContentSetPath);
    customerFile.setFileName(p_customerContentSetPath);

    if(!zeraFile.exists()){
        m_zeraContentSetPath="";
        retVal=false;
        m_lastError=error::FileDoesNotExist;
    }

    if(!customerFile.exists()){
        //m_customerContentSetPath="";
        //retVal=false;
        //m_lastError=error::FileDoesNotExist;
    }


    return retVal;
}

QMap<QString,QVector<QString>> JsonContentSetLoader::readContentSet(const QString &p_contentSetName)
{
    QMap<QString,QVector<QString>> retVal;
    try {
        if(hasContentSet(m_zeraContentSetPath,p_contentSetName)){
            retVal=readContentSetFromFile(m_zeraContentSetPath,p_contentSetName);
        }else if(hasContentSet(m_customerContentSetPath,p_contentSetName)){
            retVal=readContentSetFromFile(m_customerContentSetPath,p_contentSetName);
        }
    }  catch (error &e) {
        m_lastError=e;
    }
    return retVal;

}

bool JsonContentSetLoader::addContentSet(const QString &p_contentSetName, const QString &p_session, QMap<QString,QVector<QString>> p_entityComponentMap)
{
    bool retVal=true;
    try {
        QVector<QString> retVal;
        QFile file;
        if(m_customerContentSetPath.isEmpty()){
            throw error::FileDoesNotExist;
        }
        file.setFileName(m_customerContentSetPath);
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

        // Add new ContentSet to Session
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
        if(!session.contains(QJsonValue(p_contentSetName))){
            session.append(QJsonValue(p_contentSetName));
        }
        sessionObj.insert(p_session,session);
        rootObj.insert(c_session,sessionObj);

        // Add ContentSet
        QJsonObject contentSetObj = rootObj.value(c_contentSet).toObject();
        if(rootObj.value(c_contentSet) == QJsonValue::Undefined){
            rootObj.insert(c_contentSet,QJsonObject());
        }
        QJsonArray contentSet;
        QJsonObject element;
        for(QString tmpEnt : p_entityComponentMap.keys()){
            element.insert(c_entity,tmpEnt);
            element.insert(c_component,QJsonArray::fromStringList(p_entityComponentMap[tmpEnt].toList()));
            if(!contentSet.contains(element)){
                contentSet.append(element);
            }
        }

        contentSetObj.insert(p_contentSetName,contentSet);
        rootObj.insert(c_contentSet,contentSetObj);
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

bool JsonContentSetLoader::removeContentSet(const QString &p_contentSetName)
{
    bool retVal=true;
    try {
        QFile file;
        if(m_customerContentSetPath.isEmpty()){
            throw error::FileDoesNotExist;
        }
        file.setFileName(m_customerContentSetPath);
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
        // Remove ContentSet From Session
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
                if(ele->toString() != p_contentSetName){
                    newArr.append(ele->toString());
                }
            }
            sessionObj.insert(session,newArr);
        }

        rootObj.insert(c_session,sessionObj);


        //Remove ContentSet

        QJsonObject contentSetObj = rootObj.value(c_contentSet).toObject();
        contentSetObj.remove(p_contentSetName);

        rootObj.insert(c_contentSet,contentSetObj);

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


QVector<QString> JsonContentSetLoader::contentSetList(const QString &p_session)
{
    QSet<QString> retVal;
    try{
        retVal.unite(zeraContentSetList(p_session).toList().toSet());
        retVal.unite(customerContentSetList(p_session).toList().toSet());
    }catch(error &e){
        m_lastError=e;
    }
    return retVal.values().toVector();
}

QVector<QString> JsonContentSetLoader::sessionList()
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

JsonContentSetLoader::error JsonContentSetLoader::readLasterror()
{
    error tmpError=m_lastError;
    m_lastError=error::NoError;
    return tmpError;
}


QVector<QString> JsonContentSetLoader::zeraContentSetList(const QString &p_session)
{
    QVector<QString> retVal;
    if(!m_zeraContentSetPath.isEmpty()){
        retVal = readContentSetListFromFile(m_zeraContentSetPath, p_session);
    }
    return retVal;
}

QVector<QString> JsonContentSetLoader::customerContentSetList(const QString &p_session)
{
    QVector<QString> retVal;
    if(!m_customerContentSetPath.isEmpty()){
        retVal = readContentSetListFromFile(m_customerContentSetPath, p_session);
    }
    return retVal;
}

QVector<QString> JsonContentSetLoader::zeraSessionList()
{
    QVector<QString> retVal;
    if(!m_zeraContentSetPath.isEmpty()){
        retVal = readSessionListFromFile(m_zeraContentSetPath);
    }
    return retVal;
}

QVector<QString> JsonContentSetLoader::customerSessionList()
{
    QVector<QString> retVal;
    if(!m_customerContentSetPath.isEmpty()){
        retVal = readSessionListFromFile(m_customerContentSetPath);
    }
    return retVal;
}

QVector<QString> JsonContentSetLoader::readSessionListFromFile(const QString &p_file)
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

QVector<QString> JsonContentSetLoader::readContentSetListFromFile(const QString &p_file, const QString &p_session)
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
            QJsonValue  contentSetList = sessionObj.toObject().value(p_session);
            if(contentSetList == QJsonValue::Undefined){
            }else{
                for(QJsonValue tmpVal : contentSetList.toArray()){
                    retVal.append(tmpVal.toString());
                }
            }

        }
    }
    file.close();
    return retVal;
}

bool JsonContentSetLoader::hasContentSet(const QString &p_file, const QString &p_contentSetName)
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
                QJsonValue contentSetObj = rootObj.value(c_contentSet);
                if(contentSetObj.isUndefined()){
                    file.close();
                    throw error::ObjectDoesNotExist;
                }
                QJsonValue  contentSetList = contentSetObj.toObject().value(p_contentSetName);
                if(contentSetList.isUndefined()){
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

QMap<QString,QVector<QString>> JsonContentSetLoader::readContentSetFromFile(const QString &p_file, const QString &p_contentSet)
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
            QJsonValue contentSetObj = rootObj.value(c_contentSet);
            if(contentSetObj == QJsonValue::Undefined){
                throw error::ObjectDoesNotExist;
            }
            QJsonValue contentSetList=contentSetObj.toObject().value(p_contentSet);
            if(contentSetList == QJsonValue::Undefined){
                throw error::ObjectDoesNotExist;
            }
            for(QJsonValue tmpVal : contentSetList.toArray()){
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
