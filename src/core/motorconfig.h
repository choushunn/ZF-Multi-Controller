#pragma once

#include <qglobal.h>

namespace mc {

// 电机物理参数配置。原为编译期常量，现改为运行时可配置，
// 由 AppConfig 从 config.json 加载并注入 KCubeMotor。
// 静态默认值作为回退，保证无配置文件时行为不变。
struct MotorConfig {
    // 设备单位 → 微米换算系数
    double positionToUm = 0.029;
    // 最小/最大位置（mm）
    double minPositionMm = 0.005;
    double maxPositionMm = 24.5;
    // 默认最大速度与加速度（设备单位）
    int defaultMaxVelocity = 2600;
    int defaultAcceleration = 4000;

    // 编译期默认值（用于无 AppConfig 场景的回退）
    static constexpr double kPositionToUm = 0.029;
    static constexpr double kMinPositionMm = 0.005;
    static constexpr double kMaxPositionMm = 24.5;
    static constexpr int kDefaultMaxVelocity = 2600;
    static constexpr int kDefaultAcceleration = 4000;

    // 用编译期默认值构造
    MotorConfig() = default;

    // 从外部参数构造
    MotorConfig(double posToUm, double minMm, double maxMm, int vel, int acc)
        : positionToUm(posToUm), minPositionMm(minMm), maxPositionMm(maxMm)
        , defaultMaxVelocity(vel), defaultAcceleration(acc) {}

    // 设备单位位置 → 微米换算（纯逻辑，无硬件依赖，MinGW 亦可单测）
    static double devicePositionToUm(int positionUnits, double positionToUm = kPositionToUm)
    {
        return positionUnits * positionToUm;
    }
};

} // namespace mc
