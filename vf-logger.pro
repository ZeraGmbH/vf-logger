#-------------------------------------------------
#
# Project created by QtCreator 2016-10-11T10:12:06
#
#-------------------------------------------------

#dependencies
VEIN_DEP_EVENT = 1
VEIN_DEP_COMP = 1
VEIN_DEP_HELPER = 1
VEIN_DEP_HASH = 1
VEIN_DEP_QML = 1

HEADERS +=\
  vein-logger_global.h \
  vl_databaselogger.h \
  vl_sqlitedb.h \
  vl_datasource.h \
  vl_qmllogger.h \
  vl_abstractloggerdb.h

QT       += sql network qml
QT       -= gui

TEMPLATE = lib

public_headers.files = $$HEADERS

TARGET = vein-logger

DEFINES += VEINLOGGER_LIBRARY

SOURCES += \
    vl_databaselogger.cpp \
    vl_sqlitedb.cpp \
    vl_datasource.cpp \
    vl_qmllogger.cpp \
    vl_abstractloggerdb.cpp



# psql is currently not supported
OTHER_FILES += psql/* \
    vl_postgresdatabase.cpp \
    vl_postgresdatabase.h \
    docker/* \
    sqlite/*

unix {
    target.path = /usr/lib
    INSTALLS += target
}


RESOURCES += \
    vf_logger_data.qrc

exists( ../../vein-framework.pri ) {
  include(../../vein-framework.pri)
}
