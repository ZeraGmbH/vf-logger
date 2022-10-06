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
    m_lastError(error::NoError)
{

}

bool JsonContentSetLoader::init(const QString &p_zeraContentSetPath)
{
    bool retVal=true;

    m_zeraContentSetPath = p_zeraContentSetPath;

    QFile zeraFile;
    zeraFile.setFileName(p_zeraContentSetPath);

    if(!zeraFile.exists()){
        m_zeraContentSetPath="";
        retVal=false;
        m_lastError=error::FileDoesNotExist;
    }

    return retVal;
}

QMap<QString,QVector<QString>> JsonContentSetLoader::readContentSet(const QString &p_contentSetName)
{
    QMap<QString,QVector<QString>> retVal;
    try {
        if(hasContentSet(m_zeraContentSetPath,p_contentSetName)){
            retVal=readContentSetFromFile(m_zeraContentSetPath,p_contentSetName);
        }
    }  catch (error &e) {
        m_lastError=e;
    }
    return retVal;

}


QVector<QString> JsonContentSetLoader::contentSetList(const QString &p_session)
{
    QSet<QString> retVal;
    try{
        QStringList listZeraContexts = zeraContentSetList(p_session).toList();
        retVal.unite(QSet<QString>(listZeraContexts.begin(), listZeraContexts.end()));
    }catch(error &e){
        m_lastError=e;
    }
    return retVal.values().toVector();
}

QVector<QString> JsonContentSetLoader::zeraContentSetList(const QString &p_session)
{
    QVector<QString> retVal;
    if(!m_zeraContentSetPath.isEmpty()){
        retVal = readContentSetListFromFile(m_zeraContentSetPath, p_session);
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
                QJsonArray compListArray = tmpVal.toObject().value(c_component).toArray();
                if(compListArray.count() > 0) {
                    for(QJsonValue comp : compListArray){
                        retVal[tmpVal.toObject().value(c_entity).toString()].append(comp.toString());
                    }
                }
                else {
                    retVal[tmpVal.toObject().value(c_entity).toString()] = QVector<QString>();
                }
            }
        }
    }
    file.close();
    return retVal;
}
