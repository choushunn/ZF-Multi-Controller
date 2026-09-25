#pragma once

#include <QObject>
#include <QString>
#include <QMutex>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDateTime>

namespace mc {

// 日志级别，从低到高
enum class LogLevel {
    Debug = 0,
    Info,
    Warn,
    Error,
};

// Logger：统一日志基座，支持控制台输出 + 按日期/大小轮转文件持久化。
// 线程安全，可被 core / app / cli 各层共用。
// 设计目标：替代散落的 emit logMessage(QString)，提供分级与持久化，
// 同时保留与现有 logMessage 信号的兼容（Logger::message 信号）。
class Logger : public QObject {
    Q_OBJECT

public:
    static Logger &instance();

    // 初始化日志系统
    // dir: 日志文件目录; level: 最低输出级别; maxFileSize: 单文件最大字节数
    void init(const QString &dir = QStringLiteral("logs"),
              LogLevel level = LogLevel::Info,
              qint64 maxFileSize = 5 * 1024 * 1024); // 默认 5MB

    void setLevel(LogLevel level);
    LogLevel level() const { return level_; }

    // 主日志接口
    void log(LogLevel level, const QString &text);

    // 便捷方法
    void debug(const QString &msg) { log(LogLevel::Debug, msg); }
    void info(const QString &msg) { log(LogLevel::Info, msg); }
    void warn(const QString &msg) { log(LogLevel::Warn, msg); }
    void error(const QString &msg) { log(LogLevel::Error, msg); }

    // 关闭日志文件
    void shutdown();

    // 级别转字符串
    static QString levelToString(LogLevel level);
    static LogLevel levelFromString(const QString &str);

signals:
    // 所有日志消息都会发射此信号，供 UI / CLI 订阅刷新
    void message(LogLevel level, const QString &formatted);

private:
    Logger() = default;
    ~Logger() override;
    Q_DISABLE_COPY_MOVE(Logger)

    void rotateIfNeeded();
    QString currentLogFile() const;

    QMutex mutex_;
    LogLevel level_ = LogLevel::Info;
    QString logDir_;
    QFile file_;
    qint64 maxFileSize_ = 5 * 1024 * 1024;
    bool initialized_ = false;
};

} // namespace mc

// 便捷宏，自动附带文件名与行号
#define MC_LOG(level, msg) \
    mc::Logger::instance().log(level, QStringLiteral("[%1:%2] %3") \
        .arg(QFileInfo(__FILE__).fileName()).arg(__LINE__).arg(msg))

#define MC_DEBUG(msg) MC_LOG(mc::LogLevel::Debug, msg)
#define MC_INFO(msg)  MC_LOG(mc::LogLevel::Info, msg)
#define MC_WARN(msg)  MC_LOG(mc::LogLevel::Warn, msg)
#define MC_ERROR(msg) MC_LOG(mc::LogLevel::Error, msg)
