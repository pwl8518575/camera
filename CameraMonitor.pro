QT       += core gui multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    videoplayer.cpp \
    videoplayerslider.cpp \
    videoprocess.cpp

HEADERS += \
    mainwindow.h \
    videoplayer.h \
    videoplayerslider.h \
    videoprocess.h

INCLUDEPATH += /usr/local/include
INCLUDEPATH += /usr/local/include/opencv
INCLUDEPATH += /usr/local/include/opencv2
LIBS += -L/usr/local/lib \
 -lopencv_core \
 -lopencv_highgui \
 -lopencv_imgproc \
  -lopencv_imgcodecs \
INCLUDEPATH += /usr/include/opencv4

#LIBS += -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_videoio

#LIBS += /usr/lib/aarch64-linux-gnu/libavcodec.so.57
#LIBS += /usr/lib/aarch64-linux-gnu/libavformat.so.57
#LIBS += /usr/lib/aarch64-linux-gnu/libswscale.so.4
#LIBS += /usr/lib/aarch64-linux-gnu/libavutil.so.55
#LIBS += /usr/lib/aarch64-linux-gnu/libx264.so.152

LIBS += /usr/local/lib/libavcodec.so
LIBS += /usr/local/lib/libavformat.so
LIBS += /usr/local/lib/libswscale.so
LIBS += /usr/local/lib/libavutil.so
LIBS += /usr/local/lib/libx264.so
#LIBS += -lavcodec -lavformat -lswscale -lavutil
#LIBS += /usr/lib/x86_64-linux-gnu/oxide-qt/libffmpeg.so

FORMS += \
    mainwindow.ui \
    videoplayer.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

QMAKE_LFLAGS += -no-pie

QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.14

# 设置FFmpeg头文件的位置
INCLUDEPATH += -I /usr/local/Cellar/ffmpeg/4.3.1_1/include
# 设置FFmpeg导入库的位置
LIBS += -L /usr/local/Cellar/ffmpeg/4.3.1_1/lib  -lavutil
