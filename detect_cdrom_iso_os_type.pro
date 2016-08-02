#-------------------------------------------------
#
# Project created by QtCreator 2014-11-26T14:05:20
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET =detect_media_os_type
CONFIG   += console
CONFIG   -= app_bundle
DEFINES *=  USE_IOCTL_LINUX
DEFINES *= _LARGEFILE_SOURCE
DEFINES *= _FILE_OFFSET_BITS=64

MOC_DIR=./tmp
OBJECTS_DIR=./tmp


TEMPLATE = app


SOURCES += \
    src/cdromisoprocess.cpp \
    src/main.cpp \
    src/mediadetprocess.cpp

HEADERS += \
    src/cdromisoprocess.h \
    src/cdromisoinfo.h \
    src/mediadetglobal.h
