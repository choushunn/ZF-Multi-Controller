#include "core/motorstatus.h"

namespace mc {

MotorStatus MotorStatus::fromBits(std::uint32_t bits)
{
    MotorStatus s;
    s.channelEnabled  = (bits & 0x80000000u) != 0;
    s.movingForward   = (bits & 0x00000010u) != 0;
    s.movingReverse   = (bits & 0x00000020u) != 0;
    s.joggingForward  = (bits & 0x00000040u) != 0;
    s.joggingReverse  = (bits & 0x00000080u) != 0;
    s.homing          = (bits & 0x00000200u) != 0;
    s.homed           = (bits & 0x00000400u) != 0;
    s.forwardLimit    = (bits & 0x00000001u) != 0;
    s.reverseLimit    = (bits & 0x00000002u) != 0;
    return s;
}

QString MotorStatus::describe() const
{
    QString info = QStringLiteral("状态: ");
    if (movingForward)  info += QStringLiteral("顺时针移动 | ");
    if (movingReverse)  info += QStringLiteral("逆时针移动 | ");
    if (joggingForward) info += QStringLiteral("顺时针点动 | ");
    if (joggingReverse) info += QStringLiteral("逆时针点动 | ");
    if (homing)         info += QStringLiteral("正在归位 | ");
    if (homed)          info += QStringLiteral("已归位 | ");
    if (forwardLimit)   info += QStringLiteral("顺时针限位 | ");
    if (reverseLimit)   info += QStringLiteral("逆时针限位 | ");
    info += channelEnabled ? QStringLiteral("通道已启用 |") : QStringLiteral("通道已禁用 |");

    if (info.endsWith(QStringLiteral(" |"))) {
        info.chop(2);
    }
    return info;
}

} // namespace mc