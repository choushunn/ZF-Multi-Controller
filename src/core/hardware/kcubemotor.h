#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

#include "core/motorstatus.h"
#include "core/motorconfig.h"

// KCubeMotor 封装 Thorlabs KDC101 电机的所有原生调用（CC_*/TLI_*），
// 向上层提供面向业务的方法与信号，屏蔽厂商 API 细节。
// 上层（GUI / CLI）只依赖本类，不再直接 include 厂商头文件。
class KCubeMotor : public QObject
{
    Q_OBJECT

public:
    explicit KCubeMotor(QObject *parent = nullptr);
    ~KCubeMotor() override;

    // 用自定义配置构造
    explicit KCubeMotor(const mc::MotorConfig &config, QObject *parent = nullptr);

    // ---- 设备发现 ----
    QStringList availableDevices();

    // ---- 连接管理 ----
    bool connectTo(const QString &serialNumber);
    void disconnect();
    bool isConnected() const { return connected_; }
    QString serialNumber() const { return serial_; }

    // ---- 运动控制 ----
    bool home();
    bool stop();
    bool moveToUm(double positionUm);
    bool moveRelativeUm(double stepUm);   // 负值表示反向
    bool setVelocity(double velocityUmps, double accelerationUmps2);

    // ---- 状态查询 ----
    double positionUm() const;
    mc::MotorStatus status() const;
    bool needsHoming() const;
    double velocityUmps() const;
    double accelerationUmps2() const;

    // ---- 配置访问 ----
    const mc::MotorConfig &config() const { return config_; }
    void setConfig(const mc::MotorConfig &config) { config_ = config; }

signals:
    void logMessage(const QString &message);
    void connectedChanged(bool connected);

private:
    char serial_[16] = {0};
    bool connected_ = false;
    mc::MotorConfig config_;

    int minPositionUnits() const;
    int maxPositionUnits() const;
};

// 电机单元转换辅助：将设备单位位置转换为微米
double devicePositionToUm(int positionUnits, double positionToUm = mc::MotorConfig::kPositionToUm);
