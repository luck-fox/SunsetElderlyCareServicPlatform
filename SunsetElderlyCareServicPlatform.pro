QT       += core gui widgets charts serialport network sql

CONFIG += c++17
RC_ICONS=$$PWD/icons/icon1.ico
# 项目名称
TARGET = SunsetElderlyCarePlatform
TEMPLATE = app

# 确保SQL插件被包含
QTPLUGIN += qsqlite

# 源文件
SOURCES += main.cpp \
           healthdatamanager.cpp \
           httppushservice.cpp \
           mainwindow.cpp \
           serialdevice.cpp \
           threadworker.cpp

# 头文件
HEADERS += mainwindow.h \
           healthdatamanager.h \
           httppushservice.h \
           serialdevice.h \
           threadworker.h

# 中文适配
QMAKE_CXXFLAGS += -utf-8

RESOURCES += \
    resources.qrc
