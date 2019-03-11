#-------------------------------------------------
#
# Project created by QtCreator 2019-03-06T12:54:40
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = rtmp_pusher
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

# release版本带调试信息
QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO

win32 {
    THIRD_PARTY_INSTALL_PREFIX = D:/third_party

    INCLUDEPATH += $${THIRD_PARTY_INSTALL_PREFIX}/ffmpeg-4.1.1-win64-dev/include
    LIBS += -L$${THIRD_PARTY_INSTALL_PREFIX}/ffmpeg-4.1.1-win64-dev/lib -lavcodec \
        -L$${THIRD_PARTY_INSTALL_PREFIX}/ffmpeg-4.1.1-win64-dev/lib -lavformat \
        -L$${THIRD_PARTY_INSTALL_PREFIX}/ffmpeg-4.1.1-win64-dev/lib -lavutil \
        -L$${THIRD_PARTY_INSTALL_PREFIX}/ffmpeg-4.1.1-win64-dev/lib -lavdevice \
        -L$${THIRD_PARTY_INSTALL_PREFIX}/ffmpeg-4.1.1-win64-dev/lib -lswscale \
        -L$${THIRD_PARTY_INSTALL_PREFIX}/ffmpeg-4.1.1-win64-dev/lib -lswresample
}

macx {
    # mac only
}

unix:!macx {
    CONFIG(debug, debug|release) {
        THIRD_PARTY_INSTALL_PREFIX = /opt/third_party/debug
    } else {
        THIRD_PARTY_INSTALL_PREFIX = /opt/third_party/release
    }

    INCLUDEPATH += $${THIRD_PARTY_INSTALL_PREFIX}/ffmpeg/include
    LIBS += -L$${THIRD_PARTY_INSTALL_PREFIX}/ffmpeg/lib -lavcodec \
        -L$${THIRD_PARTY_INSTALL_PREFIX}/ffmpeg/lib -lavformat \
        -L$${THIRD_PARTY_INSTALL_PREFIX}/ffmpeg/lib -lavutil \
        -L$${THIRD_PARTY_INSTALL_PREFIX}/ffmpeg/lib -lavdevice \
        -L$${THIRD_PARTY_INSTALL_PREFIX}/ffmpeg/lib -lavfilter \
        -L$${THIRD_PARTY_INSTALL_PREFIX}/ffmpeg/lib -lpostproc \
        -L$${THIRD_PARTY_INSTALL_PREFIX}/ffmpeg/lib -lswscale \
        -L$${THIRD_PARTY_INSTALL_PREFIX}/ffmpeg/lib -lswresample \
        -L$${THIRD_PARTY_INSTALL_PREFIX}/x264/lib -lx264 \
        -L$${THIRD_PARTY_INSTALL_PREFIX}/fdk_aac/lib -lfdk-aac
}

SOURCES += \
        main.cpp \
        main_window.cpp \
    av_input_stream.cpp \
    av_output_stream.cpp \
    mf_av_devices.cpp \
    ds_av_devices.cpp

HEADERS += \
        main_window.h \
    av_input_stream.h \
    av_output_stream.h \
    mf_av_devices.h \
    ds_av_devices.h

FORMS += \
        main_window.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
