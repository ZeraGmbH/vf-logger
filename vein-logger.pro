#-------------------------------------------------
#
# Project created by QtCreator 2016-10-11T10:12:06
#
#-------------------------------------------------

#dependencies
VEIN_DEP_EVENT = 1
VEIN_DEP_COMP = 1
VEIN_DEP_HELPER = 1


QT       += sql

QT       -= gui

TARGET = vein-logger
TEMPLATE = lib

DEFINES += VEINLOGGER_LIBRARY

SOURCES += vl_datalogger.cpp \
    vl_postgresdatabase.cpp

HEADERS += vl_datalogger.h\
        vein-logger_global.h \
    vl_postgresdatabase.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}

exists( ../../vein-framework.pri ) {
  include(../../vein-framework.pri)
}
