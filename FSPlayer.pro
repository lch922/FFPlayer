QT += core
QT -= gui
DESTDIR = bin
#CONFIG += c++11
INCLUDEPATH += include include/SDL
LIBS += -LL:/Qt_Pro/FSplayer-master/FFPlayer/lib -lSDL -lSDLmain -lavcodec -lavdevice -lavfilter \
        -lavformat -lavutil -lpostproc \
        -lswresample -lswscale

DEFINES += __STDC_CONSTANT_MACROS
#DEFINES +=  SDL_MAIN_HANDLED
TARGET = FSPlayer
#CONFIG += console
#CONFIG -= app_bundle

TEMPLATE = app

SOURCES += \
#    main.cpp \
#    sdl_video.cpp \
#    simplest_ffmpeg_audio_player.cpp \
#    myPlay.cpp \
#    Audio.cpp \
#    FrameQueue.cpp \
#    Media.cpp \
#    PacketQueue.cpp \
#    Video.cpp \
#    VideoDisplay.cpp \
#    avpacket.cpp \
#    ffplay.cpp \
#    ffutils.cpp
    SynchingVideo.cpp

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

#HEADERS += \
#    Audio.h \
#    FrameQueue.h \
#    Media.h \
#    PacketQueue.h \
#    Video.h \
#    VideoDisplay.h \
#    avpacket.h \
#    log.h \
#    ffutils.h
