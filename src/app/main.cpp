#include "mainwindow.h"

#include <QApplication>
#include <QIcon>

#include "core/appconfig.h"
#include "core/logger.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName(QStringLiteral("Multi-Controller"));
    a.setApplicationVersion(QStringLiteral("0.1.0"));
    a.setOrganizationName(QStringLiteral("Multi-Controller"));

    // 加载配置
    auto &config = mc::AppConfig::instance();
    config.load();

    // 初始化日志系统
    auto &logger = mc::Logger::instance();
    logger.init(config.data().logDir,
                mc::Logger::levelFromString(config.data().logLevel),
                config.data().logMaxFileSize);

    MC_INFO(QStringLiteral("Multi-Controller v%1 启动").arg(a.applicationVersion()));

    // 设置窗口图标（.rc 已挂载到 exe，此处兜底）
    a.setWindowIcon(QIcon(QStringLiteral(":/icons/app.ico")));

    MainWindow w;
    w.showFullScreen();  // 默认全屏，可用"视图→全屏模式"或 F11/Esc 退出
    return a.exec();
}
