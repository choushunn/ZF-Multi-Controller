#-------------------------------------------------
#
# Project created by QtCreator 2016-08-29T20:11:40
#
#-------------------------------------------------

QT       += core gui
INCLUDEPATH += /home/QTCreatorPro/
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = BasicFunction
TEMPLATE = app


QMAKE_LFLAGS += -Wl,-rpath=.

SOURCES += main.cpp\
        BasicFunction.cpp \
    ImageAcquisition.cpp

HEADERS  += BasicFunction.h \
    ImageAcquisition.h \
    DVPCamera.h

FORMS    += BasicFunction.ui


win32{
    contains(QMAKE_HOST.arch, x86_64) {
        message("x86_64 build")
        ## Windows x64 (64bit) specific build here
        win32: LIBS += -L$$PWD/lib_x86_64/ -lDVPCamera64
        INCLUDEPATH += $$PWD/lib_x86_64
        DEPENDPATH += $$PWD/lib_x86_64
    } else {
        message("x86 32 build")
        ## Windows x86 (32bit) specific build here
        win32: LIBS += -L$$PWD/lib_x86_32/ -lDVPCamera
        INCLUDEPATH += $$PWD/lib_x86_32
        DEPENDPATH += $$PWD/lib_x86_32
    }
}

#Linux x86平台库配置,右键项目配置库


#Arm 平台库配置，右键项目配置库

