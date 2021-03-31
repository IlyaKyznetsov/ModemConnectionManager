QT -= gui
QT += serialport

### Library ###
TEMPLATE = app
#TEMPLATE = lib
DEFINES += MODEMCONNECTIONMANAGER_LIBRARY
# Default rules for deployment.
unix {
    target.path = /usr/lib
}
###############

### App ###
#CONFIG += c++11 console
#CONFIG -= app_bundle
## Default rules for deployment.
#qnx: target.path = /tmp/$${TARGET}/bin
#else: unix:!android: target.path = /opt/$${TARGET}/bin
###########

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

LIBS += -ludev

SOURCES += main.cpp

SOURCES += \
        Global.cpp \
        Modem.cpp \
        ModemCommander.cpp \
        ModemConnectionAutomator.cpp \
        ModemConnectionManager.cpp \
        QUdev.cpp \
        QUdevDevice.cpp

HEADERS += \
    Global.h \
    Modem.h \
    ModemCommander.h \
    ModemConnectionAutomator.h \
    ModemConnectionManager_global.h \
    ModemConnectionManager.h \
    QUdev.h \
    QUdevDevice.h

RESOURCES += \
    resources.qrc

!isEmpty(target.path): INSTALLS += target
