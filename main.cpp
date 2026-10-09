#include "mainwindow.h"
#include "logger.h"
#include <QApplication>
#include <QFont>
#include <QDebug>

int main(int argc, char *argv[])
{
    // 最先安装日志处理器，之后所有 qDebug/qInfo/qWarning/qCritical 均会落盘
    Logger::installMessageHandler();

    qDebug() << "程序启动开始...";
    QApplication a(argc, argv);

    qDebug() << "设置字体...";
    QFont font("微软雅黑", 14);
    a.setFont(font);

    qDebug() << "创建主窗口...";
    MainWindow w;
    
    qDebug() << "设置窗口标题...";
    w.setWindowTitle("智慧夕阳服务管理系统");
    
    qDebug() << "显示窗口...";
    w.show();
    
    qDebug() << "进入事件循环...";

    return a.exec();
}
