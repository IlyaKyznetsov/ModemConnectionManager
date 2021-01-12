QT -= gui

### Library ###
TEMPLATE = lib
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

SOURCES += \
        Global.cpp \
        ModemConnectionAutomator.cpp \
        ModemConnectionManager.cpp \
        main.cpp

HEADERS += \
    Global.h \
    ModemConnectionAutomator.h \
    ModemConnectionManager_global.h \
    ModemConnectionManager.h

RESOURCES += \
    resources.qrc

!isEmpty(target.path): INSTALLS += target
