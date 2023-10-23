#-------------------------------------------------
#
# Project created by QtCreator 2016-12-16T20:35:45
#
#-------------------------------------------------

QT       += core gui
linux: QT += dbus

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = BatteryLine
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
CONFIG(debug, release|debug):DEFINES += _DEBUG

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++17

SOURCES += main.cpp \
    batteryline.cpp \
    systemhelper.cpp \
    settingdialog.cpp \
    singleinstance.cpp \
    platform/notification.cpp \
    platform/powernotify.cpp \
    platform/powerstatus.cpp

HEADERS  += batteryline.h \
    var.h \
    systemhelper.h \
    settingdialog.h \
    singleinstance.h \
    resource.h \
    platform/notification.h \
    platform/powernotify.h \
    platform/powerstatus.h

FORMS    += batteryline.ui \
    settingdialog.ui

RESOURCES += batteryline.qrc

# http://doc.qt.io/qt-5/appicon.html
RC_FILE = resource.rc

win32 {
    LIBS += kernel32.lib user32.lib comctl32.lib shell32.lib
}
