#-------------------------------------------------
#
# Project created by QtCreator 2013-09-14T14:21:15
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QT       += network

TARGET = DissectorGUI
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    shaderdebugwindow.cpp \
    textureviewerfast.cpp \
    meshviewer.cpp \
    launchdialog.cpp \
    shaderanalyzer.cpp

HEADERS  += mainwindow.h \
    shaderdebugwindow.h \
    textureviewerfast.h \
    meshviewer.h \
    launchdialog.h \
    shaderanalyzer.h

FORMS    += mainwindow.ui \
    shaderdebugwindow.ui \
    textureviewerfast.ui \
    launchdialog.ui

INCLUDEPATH += "../Dissector/"
