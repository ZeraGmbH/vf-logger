#-------------------------------------------------
#
# Project created by QtCreator 2016-10-11T10:12:06
#
#-------------------------------------------------

#dependencies
VEIN_DEP_EVENT = 1
VEIN_DEP_COMP = 1
VEIN_DEP_HELPER = 1

HEADERS +=\
  vein-logger_global.h \
  vl_postgresdatabase.h \
    vl_databaselogger.h

QT       += sql
QT       -= gui

TEMPLATE = lib

public_headers.files = $$HEADERS

TARGET = vein-logger

DEFINES += VEINLOGGER_LIBRARY

SOURCES += \
    vl_postgresdatabase.cpp \
    vl_databaselogger.cpp


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
