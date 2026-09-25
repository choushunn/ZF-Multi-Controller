#pragma once

#include <cstdint>
#include <QString>

namespace mc {

// 设备状态位的具名定义，替代原先散落在 UI 层的神秘掩码。
// 值遵循 Thorlabs KDC101 状态寄存器约定。
struct MotorStatus {
    // 通道使能（0x80000000）
    bool channelEnabled = false;
    // 正在顺时针移动（0x10）
    bool movingForward = false;
    // 正在逆时针移动（0x20）
    bool movingReverse = false;
    // 正在顺时针点动（0x40）
    bool joggingForward = false;
    // 正在逆时针点动（0x80）
    bool joggingReverse = false;
    // 正在归位（0x200）
    bool homing = false;
    // 已完成归位（0x400）
    bool homed = false;
    // 顺向限位触发（0x1）
    bool forwardLimit = false;
    // 逆向限位触发（0x2）
    bool reverseLimit = false;

    // 是否处于运动过程中（移动、点动或归位）
    bool isMoving() const {
        return movingForward || movingReverse || joggingForward || joggingReverse || homing;
    }

    // 从原始状态位构造
    [[nodiscard]] static MotorStatus fromBits(std::uint32_t bits);

    // 人类可读描述，用于日志或状态栏
    [[nodiscard]] QString describe() const;
};

} // namespace mc