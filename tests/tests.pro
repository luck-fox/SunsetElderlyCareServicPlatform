# 单元测试项目 — 在 Qt Creator 中单独打开此 .pro 构建运行
# 覆盖 HealthDataManager 的 CRUD/统计/异常检测 + HttpPushService 的队列/告警去重逻辑

QT += core sql network testlib
CONFIG += c++17 console
CONFIG -= app_bundle
QMAKE_CXXFLAGS += -utf-8

TARGET = tst_sunsettests

INCLUDEPATH += $$PWD/..

# 引用主项目源码（直接编译进测试可执行文件）
SOURCES += \
    tst_main.cpp \
    ../healthdatamanager.cpp \
    ../httppushservice.cpp

HEADERS += \
    ../healthdatamanager.h \
    ../httppushservice.h
