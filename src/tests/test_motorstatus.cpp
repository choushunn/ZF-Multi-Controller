#include <QtTest>
#include "core/motorstatus.h"

class TestMotorStatus : public QObject {
    Q_OBJECT
private slots:
    void testFromBitsChannelEnabled();
    void testFromBitsMovingForward();
    void testFromBitsMovingReverse();
    void testFromBitsHoming();
    void testFromBitsHomed();
    void testFromBitsForwardLimit();
    void testFromBitsReverseLimit();
    void testFromBitsCombined();
    void testFromBitsZero();
    void testIsMoving();
    void testDescribe();
};

void TestMotorStatus::testFromBitsChannelEnabled()
{
    mc::MotorStatus s = mc::MotorStatus::fromBits(0x80000000);
    QVERIFY(s.channelEnabled);
    QVERIFY(!s.movingForward);
}

void TestMotorStatus::testFromBitsMovingForward()
{
    mc::MotorStatus s = mc::MotorStatus::fromBits(0x10);
    QVERIFY(s.movingForward);
    QVERIFY(!s.movingReverse);
}

void TestMotorStatus::testFromBitsMovingReverse()
{
    mc::MotorStatus s = mc::MotorStatus::fromBits(0x20);
    QVERIFY(s.movingReverse);
    QVERIFY(!s.movingForward);
}

void TestMotorStatus::testFromBitsHoming()
{
    mc::MotorStatus s = mc::MotorStatus::fromBits(0x200);
    QVERIFY(s.homing);
    QVERIFY(!s.homed);
}

void TestMotorStatus::testFromBitsHomed()
{
    mc::MotorStatus s = mc::MotorStatus::fromBits(0x400);
    QVERIFY(s.homed);
    QVERIFY(!s.homing);
}

void TestMotorStatus::testFromBitsForwardLimit()
{
    mc::MotorStatus s = mc::MotorStatus::fromBits(0x1);
    QVERIFY(s.forwardLimit);
    QVERIFY(!s.reverseLimit);
}

void TestMotorStatus::testFromBitsReverseLimit()
{
    mc::MotorStatus s = mc::MotorStatus::fromBits(0x2);
    QVERIFY(s.reverseLimit);
    QVERIFY(!s.forwardLimit);
}

void TestMotorStatus::testFromBitsCombined()
{
    // 通道启用 + 已归位 + 正在顺时针移动
    const std::uint32_t bits = 0x80000000 | 0x400 | 0x10;
    mc::MotorStatus s = mc::MotorStatus::fromBits(bits);
    QVERIFY(s.channelEnabled);
    QVERIFY(s.homed);
    QVERIFY(s.movingForward);
    QVERIFY(!s.movingReverse);
    QVERIFY(!s.homing);
}

void TestMotorStatus::testFromBitsZero()
{
    mc::MotorStatus s = mc::MotorStatus::fromBits(0);
    QVERIFY(!s.channelEnabled);
    QVERIFY(!s.movingForward);
    QVERIFY(!s.homed);
}

void TestMotorStatus::testIsMoving()
{
    mc::MotorStatus s;
    QVERIFY(!s.isMoving());

    s.movingForward = true;
    QVERIFY(s.isMoving());

    s.movingForward = false;
    s.homing = true;
    QVERIFY(s.isMoving());

    s.homing = false;
    s.joggingReverse = true;
    QVERIFY(s.isMoving());
}

void TestMotorStatus::testDescribe()
{
    mc::MotorStatus s;
    s.channelEnabled = true;
    s.homed = true;
    const QString desc = s.describe();
    QVERIFY(desc.contains(QStringLiteral("已归位")));
    QVERIFY(desc.contains(QStringLiteral("通道已启用")));
}

QTEST_APPLESS_MAIN(TestMotorStatus)
#include "test_motorstatus.moc"
