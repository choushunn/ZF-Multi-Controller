#include <QtTest>
#include "core/motorconfig.h"
#include "core/kcubemotor.h"

class TestMotorConfig : public QObject {
    Q_OBJECT
private slots:
    void testDefaultValues();
    void testCustomConfig();
    void testDevicePositionToUm();
    void testMinPositionUnitsCalc();
    void testMaxPositionUnitsCalc();
    void testLimitValidation_data();
    void testLimitValidation();
};

void TestMotorConfig::testDefaultValues()
{
    mc::MotorConfig cfg;
    QCOMPARE(cfg.positionToUm, 0.029);
    QCOMPARE(cfg.minPositionMm, 0.005);
    QCOMPARE(cfg.maxPositionMm, 24.5);
    QCOMPARE(cfg.defaultMaxVelocity, 2600);
    QCOMPARE(cfg.defaultAcceleration, 4000);
}

void TestMotorConfig::testCustomConfig()
{
    mc::MotorConfig cfg(0.05, 1.0, 50.0, 3000, 5000);
    QCOMPARE(cfg.positionToUm, 0.05);
    QCOMPARE(cfg.minPositionMm, 1.0);
    QCOMPARE(cfg.maxPositionMm, 50.0);
    QCOMPARE(cfg.defaultMaxVelocity, 3000);
    QCOMPARE(cfg.defaultAcceleration, 5000);
}

void TestMotorConfig::testDevicePositionToUm()
{
    // 0.029 μm per unit
    QCOMPARE(devicePositionToUm(0, 0.029), 0.0);
    QCOMPARE(devicePositionToUm(1, 0.029), 0.029);
    QCOMPARE(devicePositionToUm(100, 0.029), 2.9);
    QCOMPARE(devicePositionToUm(1000, 0.029), 29.0);
}

void TestMotorConfig::testMinPositionUnitsCalc()
{
    // minPositionMm=0.005, positionToUm=0.029
    // minPositionUnits = (0.005 * 1000) / 0.029 = 172.41... → 172
    const mc::MotorConfig cfg;
    const int expected = static_cast<int>((cfg.minPositionMm * 1000) / cfg.positionToUm);
    QCOMPARE(expected, 172); // truncated to int
}

void TestMotorConfig::testMaxPositionUnitsCalc()
{
    // maxPositionMm=24.5, positionToUm=0.029
    // maxPositionUnits = (24.5 * 1000) / 0.029 = 844827.58... → 844827
    const mc::MotorConfig cfg;
    const int expected = static_cast<int>((cfg.maxPositionMm * 1000) / cfg.positionToUm);
    QCOMPARE(expected, 844827); // truncated to int
}

void TestMotorConfig::testLimitValidation_data()
{
    QTest::addColumn<double>("positionUm");
    QTest::addColumn<bool>("expectedValid");

    // 有效范围: 5μm (0.005mm) ~ 24500μm (24.5mm)
    QTest::newRow("min boundary") << 5.0 << true;
    QTest::newRow("max boundary") << 24500.0 << true;
    QTest::newRow("mid range") << 12000.0 << true;
    QTest::newRow("below min") << 4.0 << false;
    QTest::newRow("above max") << 25000.0 << false;
    QTest::newRow("zero") << 0.0 << false;
    QTest::newRow("negative") << -100.0 << false;
}

void TestMotorConfig::testLimitValidation()
{
    QFETCH(double, positionUm);
    QFETCH(bool, expectedValid);

    const mc::MotorConfig cfg;
    const int position = static_cast<int>(positionUm / cfg.positionToUm);
    const int minPos = static_cast<int>((cfg.minPositionMm * 1000) / cfg.positionToUm);
    const int maxPos = static_cast<int>((cfg.maxPositionMm * 1000) / cfg.positionToUm);
    const bool valid = (position >= minPos && position <= maxPos);
    QCOMPARE(valid, expectedValid);
}

QTEST_APPLESS_MAIN(TestMotorConfig)
#include "test_motorconfig.moc"
