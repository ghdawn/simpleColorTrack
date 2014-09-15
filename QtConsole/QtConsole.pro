#-------------------------------------------------
#
# Project created by QtCreator 2014-09-14T19:29:07
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = 1234
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui
QT+=network

unix:!macx: LIBS += -L$$PWD/../../iTRLib/3rdparty/alglib/bin/debug/ -lalglib

INCLUDEPATH += $$PWD/../../iTRLib/3rdparty/alglib
DEPENDPATH += $$PWD/../../iTRLib/3rdparty/alglib

unix:!macx: PRE_TARGETDEPS += $$PWD/../../iTRLib/3rdparty/alglib/bin/debug/libalglib.a

unix:!macx: LIBS += -L$$PWD/../../iTRLib/itrbase/bin/debug/ -litrbase

INCLUDEPATH += $$PWD/../../iTRLib/itrbase
DEPENDPATH += $$PWD/../../iTRLib/itrbase

unix:!macx: PRE_TARGETDEPS += $$PWD/../../iTRLib/itrbase/bin/debug/libitrbase.a
