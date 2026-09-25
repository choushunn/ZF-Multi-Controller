#include "core/logger.h"

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

namespace mc {

Logger &Logger::instance()
{
    static Logger inst;
    return inst;
}

Logger::~Logger()
{
    shutdown();
}

void Logger::init(const QString &dir, LogLevel level, qint64 maxFileSize)
{
    QString path;
    {
        QMutexLocker lock(&mutex_);

        level_ = level;
        maxFileSize_ = maxFileSize;

        // 日志目录：优先使用参数指定，其次可执行文件旁的 logs/
        if (dir.isEmpty() || QDir::isRelativePath(dir))
            logDir_ = QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(dir.isEmpty() ? QStringLiteral("logs") : dir);
        else
            logDir_ = dir;

        QDir().mkpath(logDir_);

        // 打开当日日志文件
        path = currentLogFile();
        file_.setFileName(path);
        file_.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);

        initialized_ = true;
    }

    // 同时输出到控制台
    qInstallMessageHandler(nullptr); // 重置 Qt 自身消息处理器

    // 注意：不能在持锁状态下调用 log()（log 内部会再次加锁，造成自死锁）
    log(LogLevel::Info, QStringLiteral("日志系统已初始化: %1").arg(path));
}

void Logger::setLevel(LogLevel level)
{
    QMutexLocker lock(&mutex_);
    level_ = level;
}

void Logger::log(LogLevel level, const QString &text)
{
    // 格式化：[时间] [级别] 消息
    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
    const QString levelStr = levelToString(level);
    const QString formatted = QStringLiteral("[%1] [%2] %3").arg(timestamp, levelStr, text);

    QMutexLocker lock(&mutex_);

    // 级别过滤
    if (level < level_)
        return;

    // 控制台输出
    QTextStream(stdout) << formatted << Qt::endl;

    // 文件持久化
    if (initialized_ && file_.isOpen()) {
        rotateIfNeeded();
        QTextStream ts(&file_);
        ts << formatted << Qt::endl;
        file_.flush();
    }

    // 发射信号供 UI 订阅
    emit message(level, formatted);
}

void Logger::shutdown()
{
    QMutexLocker lock(&mutex_);
    if (file_.isOpen()) {
        file_.flush();
        file_.close();
    }
    initialized_ = false;
}

void Logger::rotateIfNeeded()
{
    if (file_.size() < maxFileSize_)
        return;

    // 轮转：当前文件重命名为带时间戳的备份
    file_.close();

    const QString oldPath = file_.fileName();
    const QString backup = oldPath + QStringLiteral(".") +
        QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss"));
    QFile::rename(oldPath, backup);

    // 清理旧备份：只保留最近 10 个
    QDir dir(logDir_);
    const QStringList backups = dir.entryList({QFileInfo(file_.fileName()).fileName() + QStringLiteral(".*")},
                                              QDir::Files, QDir::Time);
    for (int i = 10; i < backups.size(); ++i)
        dir.remove(backups[i]);

    // 重新打开当前日志文件
    file_.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
}

QString Logger::currentLogFile() const
{
    const QString today = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd"));
    return QDir(logDir_).absoluteFilePath(QStringLiteral("multi-controller_%1.log").arg(today));
}

QString Logger::levelToString(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug: return QStringLiteral("DEBUG");
    case LogLevel::Info:  return QStringLiteral("INFO ");
    case LogLevel::Warn:  return QStringLiteral("WARN ");
    case LogLevel::Error: return QStringLiteral("ERROR");
    }
    return QStringLiteral("?????");
}

LogLevel Logger::levelFromString(const QString &str)
{
    const QString s = str.trimmed().toLower();
    if (s == QStringLiteral("debug")) return LogLevel::Debug;
    if (s == QStringLiteral("warn") || s == QStringLiteral("warning")) return LogLevel::Warn;
    if (s == QStringLiteral("error")) return LogLevel::Error;
    return LogLevel::Info; // 默认
}

} // namespace mc
