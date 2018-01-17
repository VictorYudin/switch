TEMPLATE = app
TARGET = switch
INCLUDEPATH += .
QT += qml quick

SOURCES += \
    window.cpp \
    render.cpp \
    model.cpp \
    qtmain.cpp

HEADERS += \
    window.h \
    render.h \
    model.h

RESOURCES += switch.qrc

QML_FILES += switch.qml SwitchButton.qml

LIBS += \
    -LC:\Temp\saturn-build\lib\tbb\lib \
    C:\Temp\saturn-build\lib\opensubd\lib\osdCPU.lib \
    C:\Temp\saturn-build\lib\boost\lib\libboost_regex.lib \
    C:\Temp\saturn-build\lib\boost\lib\libboost_system.lib \
    C:\Temp\saturn-build\lib\boost\lib\libboost_filesystem.lib \
    -WHOLEARCHIVE:..\saturn-build\lib\usd\lib\usdGeom.lib \
    -WHOLEARCHIVE:..\saturn-build\lib\usd\lib\usd.lib \
    -WHOLEARCHIVE:..\saturn-build\lib\usd\lib\usdUtils.lib \
    -WHOLEARCHIVE:..\saturn-build\lib\usd\lib\tracelite.lib \
    -WHOLEARCHIVE:..\saturn-build\lib\usd\lib\pcp.lib \
    -WHOLEARCHIVE:..\saturn-build\lib\usd\lib\sdf.lib \
    -WHOLEARCHIVE:..\saturn-build\lib\usd\lib\plug.lib \
    -WHOLEARCHIVE:..\saturn-build\lib\usd\lib\js.lib \
    -WHOLEARCHIVE:..\saturn-build\lib\usd\lib\ar.lib \
    -WHOLEARCHIVE:..\saturn-build\lib\usd\lib\work.lib \
    -WHOLEARCHIVE:..\saturn-build\lib\usd\lib\tf.lib \
    -WHOLEARCHIVE:..\saturn-build\lib\usd\lib\kind.lib \
    -WHOLEARCHIVE:..\saturn-build\lib\usd\lib\arch.lib \
    -WHOLEARCHIVE:..\saturn-build\lib\usd\lib\vt.lib \
    -WHOLEARCHIVE:..\saturn-build\lib\usd\lib\gf.lib \
    Shlwapi.lib Dbghelp.lib

INCLUDEPATH += \
    C:\Temp\saturn-build\lib\opensubd\include \
    C:\Temp\saturn-build\lib\usd\include \
    C:\Temp\saturn-build\lib\boost\include \
    C:\Temp\saturn-build\lib\tbb\include

DEFINES += NOMINMAX BOOST_ALL_NO_LIB
QMAKE_CXXFLAGS += /wd4100 /wd4244 /wd4305
QMAKE_CFLAGS_WARN_ON -= -W3
QMAKE_CXXFLAGS_WARN_ON -= -W3

target.path = c:\temp\switch-bin
INSTALLS += target
