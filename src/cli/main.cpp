#include <QCoreApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QTextStream>

#include "core/kcubemotor.h"
#include "core/appconfig.h"
#include "core/logger.h"

namespace {
void logLine(const QString &message)
{
    QTextStream(stdout) << message << Qt::endl;
}
} // namespace

// Multi-Controller 命令行入口：与 GUI 复用同一底层 KCubeMotor，
// 用于脚本化 / 自动化的电机控制与状态查询。
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("Multi-ControllerCLI"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    // 加载配置
    auto &config = mc::AppConfig::instance();
    config.load();

    // 初始化日志（CLI 模式：仅控制台，但可选文件）
    auto &logger = mc::Logger::instance();
    logger.init(config.data().logDir,
                mc::Logger::levelFromString(config.data().logLevel),
                config.data().logMaxFileSize);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Multi-Controller 命令行电机控制"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption listOpt(QStringLiteral("list"),
                               QStringLiteral("列出可用设备并退出"));
    QCommandLineOption serialOpt(QStringLiteral("s"),
                                 QStringLiteral("指定设备序列号（默认自动选择第一个可用设备）"),
                                 QStringLiteral("serial"));
    QCommandLineOption homeOpt(QStringLiteral("home"), QStringLiteral("执行归位"));
    QCommandLineOption stopOpt(QStringLiteral("stop"), QStringLiteral("停止电机"));
    QCommandLineOption moveOpt(QStringLiteral("m"),
                               QStringLiteral("移动到指定位置（微米）"),
                               QStringLiteral("um"));
    QCommandLineOption relOpt(QStringLiteral("r"),
                              QStringLiteral("相对移动（微米，负值反向）"),
                              QStringLiteral("um"));
    QCommandLineOption statusOpt(QStringLiteral("status"), QStringLiteral("查询并打印状态"));
    QCommandLineOption velocityOpt(QStringLiteral("velocity"),
                                   QStringLiteral("设置最大速度（微米/秒）"),
                                   QStringLiteral("um/s"));
    QCommandLineOption accelerationOpt(QStringLiteral("acceleration"),
                                       QStringLiteral("设置加速度（微米/秒²）"),
                                       QStringLiteral("um/s2"));

    parser.addOption(listOpt);
    parser.addOption(serialOpt);
    parser.addOption(homeOpt);
    parser.addOption(stopOpt);
    parser.addOption(moveOpt);
    parser.addOption(relOpt);
    parser.addOption(statusOpt);
    parser.addOption(velocityOpt);
    parser.addOption(accelerationOpt);
    parser.process(app);

    // 用配置参数构造电机
    const auto &cfg = config.data();
    KCubeMotor motor(mc::MotorConfig(cfg.positionToUm, cfg.minPositionMm,
                                    cfg.maxPositionMm, cfg.defaultMaxVelocity,
                                    cfg.defaultAcceleration));
    QObject::connect(&motor, &KCubeMotor::logMessage, &logLine);

    // 仅列举设备
    if (parser.isSet(listOpt)) {
        const QStringList devices = motor.availableDevices();
        if (devices.isEmpty())
            logLine(QStringLiteral("未发现任何设备"));
        else {
            logLine(QStringLiteral("发现 %1 个设备:").arg(devices.size()));
            for (const QString &serial : devices)
                logLine(serial);
        }
        return 0;
    }

    // 选择并连接设备
    QString serial = parser.value(serialOpt);
    if (serial.isEmpty()) {
        // 优先用配置中的默认序列号
        serial = config.defaultSerial();
        if (serial.isEmpty()) {
            const QStringList devices = motor.availableDevices();
            if (devices.isEmpty()) {
                logLine(QStringLiteral("未发现任何设备"));
                return 1;
            }
            serial = devices.first();
            logLine(QStringLiteral("自动选择设备: %1").arg(serial));
        }
    }

    if (!motor.connectTo(serial)) {
        logLine(QStringLiteral("连接设备失败: %1").arg(serial));
        return 1;
    }

    if (parser.isSet(homeOpt))
        motor.home();

    if (parser.isSet(stopOpt))
        motor.stop();

    if (parser.isSet(velocityOpt) || parser.isSet(accelerationOpt)) {
        const double vel = parser.value(velocityOpt).toDouble();
        const double acc = parser.value(accelerationOpt).toDouble();
        motor.setVelocity(vel, acc);
    }

    if (parser.isSet(moveOpt))
        motor.moveToUm(parser.value(moveOpt).toDouble());

    if (parser.isSet(relOpt))
        motor.moveRelativeUm(parser.value(relOpt).toDouble());

    if (parser.isSet(statusOpt)) {
        logLine(QStringLiteral("当前位置: %1 μm").arg(motor.positionUm(), 0, 'f', 1));
        logLine(motor.status().describe());
    }

    motor.disconnect();
    return 0;
}
