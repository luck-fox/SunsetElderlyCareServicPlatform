#ifndef LOGGER_H
#define LOGGER_H

#include <QString>

namespace Logger {

// 安装全局消息处理器，将 Qt 日志(qDebug/qInfo/qWarning/qCritical)按级别
// 落盘到 applicationDirPath()/logs/app_<yyyy-MM-dd>.log，同时保留控制台输出。
// 应在 main() 最开头、创建 QApplication 之前调用。
void installMessageHandler();

} // namespace Logger

#endif // LOGGER_H
