QT += testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

SOURCES +=  tst_tests.cpp

unix:!macx: LIBS += -L$$OUT_PWD/../src/ -lModemConnectionManager
INCLUDEPATH += $$PWD/../src
DEPENDPATH += $$PWD/../src
