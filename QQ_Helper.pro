QT       += core gui
QT       += winextras
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
    QQChatWin.cpp \
    WinUtility.cpp \
    hook.cpp \
    icontip.cpp \
    main.cpp \
    systemtray.cpp \
    widget.cpp

HEADERS += \
    QQChatWin.h \
    WinUtility.h \
    hook.h \
    icontip.h \
    systemtray.h \
    widget.h

FORMS += \
    icontip.ui \
    widget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    readme.txt

#CONFIG (release, debug|release) { //release模式下禁用qDebug()会提升性能 刷新请采用[重新构建项目]
#    message("release mode")
#    DEFINES += QT_NO_DEBUG_OUTPUT
#}

RC_ICONS = images/icon.ico

TARGET = "QQ Follower"

LIBS += -lpsapi -luser32

RESOURCES += \
   res.qrc

msvc {
    QMAKE_CFLAGS += /utf-8
    QMAKE_CXXFLAGS += /utf-8
}
