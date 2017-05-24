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
  vl_postgresdatabase.h \
    vl_databaselogger.h \
    vl_sqlitedb.h \
    vl_datasource.h

QT       += sql network qml
QT       -= gui

TEMPLATE = lib

public_headers.files = $$HEADERS

TARGET = vein-logger

DEFINES += VEINLOGGER_LIBRARY

SOURCES += \
    vl_postgresdatabase.cpp \
    vl_databaselogger.cpp \
    vl_sqlitedb.cpp \
    vl_datasource.cpp


OTHER_FILES += psql/* \
    docker/* \
    sqlite/*

unix {
    target.path = /usr/lib
    INSTALLS += target
}


exists( ../../vein-framework.pri ) {
  include(../../vein-framework.pri)
}

RESOURCES += \
    vf_logger_data.qrc
