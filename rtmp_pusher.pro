#-------------------------------------------------
#
# Project created by QtCreator 2019-03-06T12:54:40
#
#-------------------------------------------------

QT       += core gui multimedia multimediawidgets

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

    INCLUDEPATH += $${THIRD_PARTY_INSTALL_PREFIX}/ffmpeg-4.1.1-win32-dev/include

    LIBS += -L$${THIRD_PARTY_INSTALL_PREFIX}/ffmpeg-4.1.1-win32-dev/lib -lavcodec \
        -L$${THIRD_PARTY_INSTALL_PREFIX}/ffmpeg-4.1.1-win32-dev/lib -lavformat \
        -L$${THIRD_PARTY_INSTALL_PREFIX}/ffmpeg-4.1.1-win32-dev/lib -lavutil \
        -L$${THIRD_PARTY_INSTALL_PREFIX}/ffmpeg-4.1.1-win32-dev/lib -lavdevice \
        -L$${THIRD_PARTY_INSTALL_PREFIX}/ffmpeg-4.1.1-win32-dev/lib -lswscale \
        -L$${THIRD_PARTY_INSTALL_PREFIX}/ffmpeg-4.1.1-win32-dev/lib -lswresample

    INCLUDEPATH += "C:/Program Files (x86)/Visual Leak Detector/include"
    LIBS += -L"C:/Program Files (x86)/Visual Leak Detector/lib/Win32" -lvld
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
        -L$${THIRD_PARTY_INSTALL_PREFIX}/fdk_aac/lib -lfdk-aac \
        -lasound
}

SOURCES += \
        main.cpp \
        main_window.cpp \
    av_input_stream.cpp \
    av_output_stream.cpp \
    devices_dialog.cpp \
    config.cpp \
    signal_center.cpp \
    video_capture.cpp \
    audio_capture.cpp \
    capture_device.cpp \
    frame_handler.cpp \
    video_handler.cpp \
    audio_handler.cpp

HEADERS += \
        main_window.h \
    av_input_stream.h \
    av_output_stream.h \
    devices_dialog.h \
    config.h \
    signal_center.h \
    singleton.h \
    global.h \
    video_capture.h \
    audio_capture.h \
    capture_device.h \
    frame_handler.h \
    video_handler.h \
    audio_handler.h

FORMS += \
        main_window.ui \
    devices_dialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
