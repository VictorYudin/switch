# Copyright 2018 Victor Yudin. All rights reserved.

TEMPLATE = app
TARGET = Switches
INCLUDEPATH += .
QT += qml quick

SOURCES += \
    hdrloader.cpp \
    model.cpp \
    object.cpp \
    qtmain.cpp \
    render.cpp \
    window.cpp

HEADERS += \
    hdrloader.h \
    model.h \
    object.h \
    render.h \
    window.h

RESOURCES += switches.qrc

QML_FILES += switches.qml SwitchButton.qml

QMAKE_CXXFLAGS += /wd4100 /wd4244 /wd4305
QMAKE_CFLAGS_WARN_ON -= -W3
QMAKE_CXXFLAGS_WARN_ON -= -W3

target.path = c:\temp\switch-bin
INSTALLS += target

WINRT_MANIFEST.logo_large = images/logo_150x150.png
WINRT_MANIFEST.logo_44x44 = images/logo_44x44.png
WINRT_MANIFEST.logo_620x300 = images/logo_620x300.png
WINRT_MANIFEST.logo_store = images/logo_store.png
WINRT_MANIFEST.background = images/logo_store.png
WINRT_MANIFEST.background = $${LITERAL_HASH}2B3841
WINRT_MANIFEST.capabilities = internetClient
