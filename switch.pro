TEMPLATE = app
TARGET = switch
INCLUDEPATH += .
QT += qml quick

SOURCES += \
    model.cpp \
    object.cpp \
    qtmain.cpp \
    render.cpp \
    window.cpp

HEADERS += \
    model.h \
    object.h \
    render.h \
    window.h

RESOURCES += switch.qrc

QML_FILES += switch.qml SwitchButton.qml

QMAKE_CXXFLAGS += /wd4100 /wd4244 /wd4305
QMAKE_CFLAGS_WARN_ON -= -W3
QMAKE_CXXFLAGS_WARN_ON -= -W3

target.path = c:\temp\switch-bin
INSTALLS += target
