# Add files and directories to ship with the application 
# by adapting the examples below.
#file1.source = myfile
#dir1.source = mydir
#DEPLOYMENTFOLDERS = file1 dir1
#dir1.source = islands
#DEPLOYMENTFOLDERS = dir1

# UID from Symbian Signed:
#symbian:TARGET.UID3 = 0xA00131BD
# Nokia UID for use only on Ovi Store! Not to be used anywhere else:
symbian:TARGET.UID3 = 0x200353ef

VERSION = 0.23

# Smart Installer package's UID
# This UID is from the protected range 
# and therefore the package will fail to install if self-signed
# By default qmake uses the unprotected range value if unprotected UID is defined for the application
# and 0x2002CCCF value if protected UID is given to the application
#symbian:DEPLOYMENT.installer_header = 0x2002CCCF

# SwEvent needed so that lauching the online help still works on Symbian when browser already open
symbian {
    TARGET.CAPABILITY += SwEvent

    my_deployment.pkg_prerules += vendorinfo

    DEPLOYMENT += my_deployment

    vendorinfo += "%{\"Mark Harman\"}" ":\"Mark Harman\""
}

# If your application uses the Qt Mobility libraries, uncomment
# the following lines and add the respective components to the 
# MOBILITY variable. 
CONFIG += mobility
MOBILITY += feedback
QT += phonon

SOURCES += main.cpp mainwindow.cpp \
    utils.cpp \
    resources.cpp \
    game.cpp \
    sector.cpp \
    player.cpp \
    panel.cpp \
    gui.cpp \
    gamestate.cpp \
    qt_sound.cpp \
    qt_image.cpp \
    qt_screen.cpp
HEADERS += mainwindow.h \
    utils.h \
    resources.h \
    common.h \
    qt_screen.h \
    game.h \
    qt_image.h \
    qt_sound.h \
    stdafx.h \
    sector.h \
    player.h \
    panel.h \
    gui.h \
    gamestate.h
FORMS +=

DEFINES += USING_QT

win32:LIBS += -lshlwapi -lshell32 -luser32

# Please do not modify the following two lines. Required for deployment.
include(deployment.pri)
qtcAddDeployment()

OTHER_FILES += \
    qtc_packaging/debian_harmattan/rules \
    qtc_packaging/debian_harmattan/README \
    qtc_packaging/debian_harmattan/manifest.aegis \
    qtc_packaging/debian_harmattan/copyright \
    qtc_packaging/debian_harmattan/control \
    qtc_packaging/debian_harmattan/compat \
    qtc_packaging/debian_harmattan/changelog \
    qtc_packaging/debian_fremantle/rules \
    qtc_packaging/debian_fremantle/README \
    qtc_packaging/debian_fremantle/copyright \
    qtc_packaging/debian_fremantle/control \
    qtc_packaging/debian_fremantle/compat \
    qtc_packaging/debian_fremantle/changelog

RESOURCES += \
    gigalomania.qrc
