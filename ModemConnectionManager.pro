TEMPLATE = subdirs

# build the project sequentially as listed in SUBDIRS !
CONFIG += ordered

SUBDIRS += \
    src/ModemConnectionManager.pro \
    tests/Tests.pro

# where to find the sub projects - give the folders
 ModemConnectionManager.subdir = src/ModemConnectionManager.pro
 Tests.subdir  = tests/Tests.pro

 # what subproject depends on others
 Tests.depends = ModemConnectionManager
