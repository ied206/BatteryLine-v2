#-------------------------------------------------
#
# Project created by QtCreator 2016-12-16T20:35:45
#
#-------------------------------------------------

QT       += core gui
linux: QT += dbus

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = BatteryLine-Qt
TEMPLATE = app


# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += main.cpp\
     batteryline.cpp \
    systemhelper.cpp \
    settingdialog.cpp

win32: SOURCES += platform/win/powernotify-win.cpp \
    platform/win/powerstatus-win.cpp
linux: SOURCES += platform/linux/powernotify-linux.cpp \
    platform/linux/powerstatus-linux.cpp

HEADERS  += batteryline.h \
    var.h \
    systemhelper.h \
    settingdialog.h

win32: HEADERS += platform/win/powernotify-win.h \
    platform/win/powerstatus-win.h
linux: HEADERS += platform/linux/powernotify-linux.h \
    platform/linux/powerstatus-linux.h

FORMS    += batteryline.ui \
    settingdialog.ui

RESOURCES += \
    batteryline-qt.qrc

win32 {
    LIBS += kernel32.lib user32.lib
# http://doc.qt.io/qt-5/appicon.html
    RC_ICONS = images/Cycle.ico
}

