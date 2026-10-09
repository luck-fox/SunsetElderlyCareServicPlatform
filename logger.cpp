#include "logger.h"
#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QStringConverter>
#include <cstdio>

static QFile s_logFile;
static QString s_currentDate;
static QMutex s_logMutex;

static QString levelToString(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:    return QStringLiteral("INFO");
    case QtInfoMsg:     return QStringLiteral("INFO");
    case QtWarningMsg:  return QStringLiteral("WARN");
    case QtCriticalMsg: return QStringLiteral("ERROR");
    case QtFatalMsg:    return QStringLiteral("FATAL");
    default:            return QStringLiteral("INFO");
    }
}

static void openLogForDate(const QString &dateStr)
{
    QString dirPath = QDir::currentPath() + "/logs";
    QDir().mkpath(dirPath);

    s_logFile.setFileName(dirPath + "/app_" + dateStr + ".log");
    s_logFile.open(QIODevice::WriteOnly | QIODevice::Append);
    s_currentDate = dateStr;
}

static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);
    QMutexLocker locker(&s_logMutex);

    QString nowStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString level = levelToString(type);
    QString formatted = QStringLiteral("[%1] [%2] %3").arg(nowStr, level, msg);

    // 按天轮转日志文件
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    if (today != s_currentDate) {
        if (s_logFile.isOpen()) s_logFile.close();
        openLogForDate(today);
    } else if (!s_logFile.isOpen()) {
        openLogForDate(today);
    }

    // 落盘
    if (s_logFile.isOpen()) {
        QTextStream ts(&s_logFile);
        ts.setEncoding(QStringConverter::Utf8);
        ts << formatted << "\n";
        ts.flush();
    }

    // 同时输出到控制台
    fprintf(stderr, "%s\n", formatted.toLocal8Bit().constData());
    fflush(stderr);

    if (type == QtFatalMsg) {
        abort();
    }
}

void Logger::installMessageHandler()
{
    qInstallMessageHandler(messageHandler);
    qDebug() << "[日志系统] 消息处理器已安装，日志文件位于 logs/app_<日期>.log";
}
