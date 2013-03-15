# Add files and directories to ship with the application 
# by adapting the examples below.
#file1.source = myfile
#dir1.source = mydir
#DEPLOYMENTFOLDERS = file1 dir1
#dir1.source = islands
#dir2.source = gfx
#DEPLOYMENTFOLDERS = dir1 dir2
#file1.source = readme.html
#DEPLOYMENTFOLDERS = file1
dir1.source = gfx
dir2.source = sound
dir3.source = islands
DEPLOYMENTFOLDERS += dir1 dir2 dir3

# UID from Symbian Signed:
#symbian:TARGET.UID3 = 0xA00131BD
# Nokia UID for use only on Ovi Store! Not to be used anywhere else:
symbian:TARGET.UID3 = 0x2005fff6

VERSION = 0.26

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
android {
    # feedback and phonon not supported on Android
    LIBS += -lOpenSLES
}
else {
    CONFIG += mobility
    MOBILITY += feedback + systeminfo
    QT += phonon
}

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
    qt_screen.cpp \
    androidaudio/androidsoundeffect.cpp \
    androidaudio/androidaudio.cpp
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
    gamestate.h \
    androidaudio/androidsoundeffect.h \
    androidaudio/androidaudio.h
FORMS +=

DEFINES += USING_QT

android {
    # otherwise android tries to include the win32 libs?!
}
else {
    win32:LIBS += -lshlwapi -lshell32 -luser32
}

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
    qtc_packaging/debian_fremantle/changelog \
    android/AndroidManifest.xml \
    android/res/drawable/icon.png \
    android/res/drawable/logo.png \
    android/res/drawable-hdpi/icon.png \
    android/res/drawable-ldpi/icon.png \
    android/res/drawable-mdpi/icon.png \
    android/res/layout/splash.xml \
    android/res/values/libs.xml \
    android/res/values/strings.xml \
    android/res/values-de/strings.xml \
    android/res/values-el/strings.xml \
    android/res/values-es/strings.xml \
    android/res/values-et/strings.xml \
    android/res/values-fa/strings.xml \
    android/res/values-fr/strings.xml \
    android/res/values-id/strings.xml \
    android/res/values-it/strings.xml \
    android/res/values-ja/strings.xml \
    android/res/values-ms/strings.xml \
    android/res/values-nb/strings.xml \
    android/res/values-nl/strings.xml \
    android/res/values-pl/strings.xml \
    android/res/values-pt-rBR/strings.xml \
    android/res/values-ro/strings.xml \
    android/res/values-rs/strings.xml \
    android/res/values-ru/strings.xml \
    android/res/values-zh-rCN/strings.xml \
    android/res/values-zh-rTW/strings.xml \
    android/src/org/kde/necessitas/ministro/IMinistro.aidl \
    android/src/org/kde/necessitas/ministro/IMinistroCallback.aidl \
    android/src/org/kde/necessitas/origo/QtActivity.java \
    android/src/org/kde/necessitas/origo/QtApplication.java \
    android/version.xml

RESOURCES +=
