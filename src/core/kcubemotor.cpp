#include "core/kcubemotor.h"
#include "core/logger.h"

#include "Thorlabs.MotionControl.KCube.DCServo.h"

#include <QByteArray>
#include <cstring>

using mc::MotorConfig;
using mc::Logger;
using mc::LogLevel;

double devicePositionToUm(int positionUnits, double positionToUm)
{
    return positionUnits * positionToUm;
}

KCubeMotor::KCubeMotor(QObject *parent)
    : QObject(parent), connected_(false)
{
    memset(serial_, 0, sizeof(serial_));
    TLI_InitializeSimulations();
}

KCubeMotor::KCubeMotor(const MotorConfig &config, QObject *parent)
    : QObject(parent), connected_(false), config_(config)
{
    memset(serial_, 0, sizeof(serial_));
    TLI_InitializeSimulations();
}

KCubeMotor::~KCubeMotor()
{
    if (connected_) {
        CC_StopImmediate(serial_);
        CC_Close(serial_);
    }
    TLI_UninitializeSimulations();
}

int KCubeMotor::minPositionUnits() const
{
    return static_cast<int>((config_.minPositionMm * 1000) / config_.positionToUm);
}

int KCubeMotor::maxPositionUnits() const
{
    return static_cast<int>((config_.maxPositionMm * 1000) / config_.positionToUm);
}

QStringList KCubeMotor::availableDevices()
{
    TLI_BuildDeviceList();
    QStringList devices;

    const short numDevices = TLI_GetDeviceListSize();
    if (numDevices <= 0)
        return devices;

    char deviceList[1024] = {0};
    if (TLI_GetDeviceListExt(deviceList, sizeof(deviceList)) == 0) {
        for (const QString &serial : QString(deviceList).split(',')) {
            if (!serial.isEmpty())
                devices << serial;
        }
    }
    return devices;
}

bool KCubeMotor::connectTo(const QString &serialNumber)
{
    if (connected_)
        return false;

    if (serialNumber.isEmpty())
        return false;

    const QByteArray ba = serialNumber.toLocal8Bit();
    strncpy_s(serial_, ba.constData(), sizeof(serial_) - 1);

    if (CC_Open(serial_) != 0) {
        emit logMessage(QStringLiteral("设备连接失败: %1").arg(serialNumber));
        return false;
    }

    connected_ = true;
    emit connectedChanged(true);

    if (CC_LoadSettings(serial_))
        emit logMessage(QStringLiteral("设备设置加载成功"));
    else
        emit logMessage(QStringLiteral("警告: 设备设置加载失败"));

    if (CC_EnableChannel(serial_) == 0) {
        emit logMessage(QStringLiteral("设备通道已启用"));

        const int minPos = minPositionUnits();
        const int maxPos = maxPositionUnits();
        if (CC_SetStageAxisLimits(serial_, minPos, maxPos) == 0) {
            emit logMessage(QStringLiteral("软件限位已设置: %1mm - %2mm")
                                .arg(config_.minPositionMm)
                                .arg(config_.maxPositionMm));
        } else {
            emit logMessage(QStringLiteral("警告: 设置软件限位失败"));
        }

        if (CC_SetVelParams(serial_, config_.defaultAcceleration,
                            config_.defaultMaxVelocity) == 0) {
            emit logMessage(QStringLiteral("默认速度参数已设置: 最大速度=%1, 加速度=%2")
                                .arg(config_.defaultMaxVelocity)
                                .arg(config_.defaultAcceleration));
        } else {
            emit logMessage(QStringLiteral("警告: 设置默认速度参数失败"));
        }

        if (CC_SetJogVelParams(serial_, config_.defaultAcceleration,
                               config_.defaultMaxVelocity) == 0) {
            emit logMessage(QStringLiteral("点动速度参数已设置"));
        } else {
            emit logMessage(QStringLiteral("警告: 设置点动速度参数失败"));
        }
    } else {
        emit logMessage(QStringLiteral("警告: 设备通道启用失败"));
    }

    if (CC_StartPolling(serial_, 200))
        emit logMessage(QStringLiteral("设备轮询已启动"));
    else
        emit logMessage(QStringLiteral("警告: 设备轮询启动失败"));

    if (CC_NeedsHoming(serial_))
        emit logMessage(QStringLiteral("设备需要归位后才能移动"));
    else if (CC_CanMoveWithoutHomingFirst(serial_))
        emit logMessage(QStringLiteral("设备可以在不归位的情况下移动"));
    else
        emit logMessage(QStringLiteral("设备需要归位后才能移动"));

    return true;
}

void KCubeMotor::disconnect()
{
    if (!connected_)
        return;

    if (CC_StopImmediate(serial_) != 0)
        emit logMessage(QStringLiteral("警告: 停止电机失败"));
    if (CC_DisableChannel(serial_) != 0)
        emit logMessage(QStringLiteral("警告: 禁用通道失败"));

    CC_StopPolling(serial_);
    CC_Close(serial_);

    connected_ = false;
    emit connectedChanged(false);
    emit logMessage(QStringLiteral("设备已断开连接"));
}

bool KCubeMotor::home()
{
    if (!connected_)
        return false;

    if (!CC_CanHome(serial_)) {
        emit logMessage(QStringLiteral("设备不支持归位操作"));
        return false;
    }

    if (status().isMoving()) {
        emit logMessage(QStringLiteral("设备正在移动中，请稍后再试"));
        return false;
    }

    const short result = CC_Home(serial_);
    if (result == 0) {
        emit logMessage(QStringLiteral("开始归位操作..."));
        return true;
    }
    emit logMessage(QStringLiteral("归位操作失败，错误代码: %1").arg(result));
    return false;
}

bool KCubeMotor::stop()
{
    if (!connected_)
        return false;

    if (CC_StopImmediate(serial_) == 0) {
        emit logMessage(QStringLiteral("电机已停止"));
        return true;
    }
    emit logMessage(QStringLiteral("停止电机失败"));
    return false;
}

bool KCubeMotor::moveToUm(double positionUm)
{
    if (!connected_)
        return false;

    const mc::MotorStatus s = status();
    if (s.isMoving()) {
        emit logMessage(QStringLiteral("设备正在移动中，请稍后再试"));
        return false;
    }
    if (needsHoming() && !s.homed) {
        emit logMessage(QStringLiteral("设备需要先归位才能移动"));
        return false;
    }

    const int position = static_cast<int>(positionUm / config_.positionToUm);
    if (position < minPositionUnits() || position > maxPositionUnits()) {
        emit logMessage(QStringLiteral("位置超出软件限位范围，有效范围: %1mm - %2mm")
                            .arg(config_.minPositionMm)
                            .arg(config_.maxPositionMm));
        return false;
    }

    const short result = CC_MoveToPosition(serial_, position);
    if (result == 0) {
        emit logMessage(QStringLiteral("移动到位置: %1μm").arg(positionUm, 0, 'f', 1));
        return true;
    }
    emit logMessage(QStringLiteral("移动失败，错误代码: %1").arg(result));
    return false;
}

bool KCubeMotor::moveRelativeUm(double stepUm)
{
    if (!connected_)
        return false;

    const int step = static_cast<int>(stepUm / config_.positionToUm);
    const int newPosition = CC_GetPosition(serial_) + step;

    if (newPosition > maxPositionUnits()) {
        emit logMessage(QStringLiteral("正向点动会超出软件限位范围，最大位置: %1mm")
                            .arg(config_.maxPositionMm));
        return false;
    }
    if (newPosition < minPositionUnits()) {
        emit logMessage(QStringLiteral("反向点动会超出软件限位范围，最小位置: %1mm")
                            .arg(config_.minPositionMm));
        return false;
    }

    if (CC_MoveRelative(serial_, step) == 0) {
        const double absUm = qAbs(stepUm);
        emit logMessage(QStringLiteral("%1点动: %2μm")
                            .arg(stepUm >= 0 ? QStringLiteral("正向") : QStringLiteral("反向"))
                            .arg(absUm, 0, 'f', 1));
        return true;
    }
    emit logMessage(QStringLiteral("点动失败"));
    return false;
}

bool KCubeMotor::setVelocity(double velocityUmps, double accelerationUmps2)
{
    if (!connected_)
        return false;

    int deviceVelocity = config_.defaultMaxVelocity;
    int deviceAcceleration = config_.defaultAcceleration;

    if (CC_GetDeviceUnitFromRealValue(serial_, velocityUmps, &deviceVelocity, 1) != 0) {
        emit logMessage(QStringLiteral("警告: 速度单位转换失败，使用默认值"));
    }
    if (CC_GetDeviceUnitFromRealValue(serial_, accelerationUmps2, &deviceAcceleration, 2) != 0) {
        emit logMessage(QStringLiteral("警告: 加速度单位转换失败，使用默认值"));
    }

    if (CC_SetVelParams(serial_, deviceAcceleration, deviceVelocity) == 0) {
        emit logMessage(QStringLiteral("速度参数已更新: 最大速度=%1 μm/s, 加速度=%2 μm/s²")
                            .arg(velocityUmps, 0, 'f', 2)
                            .arg(accelerationUmps2, 0, 'f', 2));
        if (CC_SetJogVelParams(serial_, deviceAcceleration, deviceVelocity) == 0)
            emit logMessage(QStringLiteral("点动速度参数已同步更新"));
        else
            emit logMessage(QStringLiteral("警告: 同步更新点动速度参数失败"));
        return true;
    }

    emit logMessage(QStringLiteral("设置速度参数失败"));
    return false;
}

double KCubeMotor::positionUm() const
{
    if (!connected_)
        return 0.0;
    return devicePositionToUm(CC_GetPosition(serial_), config_.positionToUm);
}

mc::MotorStatus KCubeMotor::status() const
{
    if (!connected_)
        return {};
    return mc::MotorStatus::fromBits(CC_GetStatusBits(serial_));
}

bool KCubeMotor::needsHoming() const
{
    if (!connected_)
        return false;
    return CC_NeedsHoming(serial_) != 0;
}

namespace {
enum : int {
    kUnitTypeVelocity = 1,
    kUnitTypeAcceleration = 2,
};
}

double KCubeMotor::velocityUmps() const
{
    if (!connected_)
        return 0.0;

    int currentVelocity = 0;
    int currentAcceleration = 0;
    if (CC_GetVelParams(serial_, &currentAcceleration, &currentVelocity) != 0)
        return config_.defaultMaxVelocity;

    double realValue = 0.0;
    if (CC_GetRealValueFromDeviceUnit(serial_, currentVelocity, &realValue, kUnitTypeVelocity) == 0)
        return realValue;
    return currentVelocity;
}

double KCubeMotor::accelerationUmps2() const
{
    if (!connected_)
        return 0.0;

    int currentVelocity = 0;
    int currentAcceleration = 0;
    if (CC_GetVelParams(serial_, &currentAcceleration, &currentVelocity) != 0)
        return config_.defaultAcceleration;

    double realValue = 0.0;
    if (CC_GetRealValueFromDeviceUnit(serial_, currentAcceleration, &realValue, kUnitTypeAcceleration) == 0)
        return realValue;
    return currentAcceleration;
}
