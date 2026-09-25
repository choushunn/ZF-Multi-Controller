#include <QtTest>
#include <QTemporaryDir>
#include "core/appconfig.h"
#include "core/logger.h"

class TestAppConfig : public QObject {
    Q_OBJECT
private slots:
    void testDefaults();
    void testSaveAndLoad();
    void testPartialLoad();
    void testLevelFromString();
};

void TestAppConfig::testDefaults()
{
    mc::AppConfigData d;
    QCOMPARE(d.defaultSerial, QString());
    QCOMPARE(d.positionToUm, 0.029);
    QCOMPARE(d.minPositionMm, 0.005);
    QCOMPARE(d.maxPositionMm, 24.5);
    QCOMPARE(d.defaultMaxVelocity, 2600);
    QCOMPARE(d.defaultAcceleration, 4000);
    QCOMPARE(d.logLevel, QStringLiteral("info"));
    QCOMPARE(d.logMaxFileSize, qint64(5 * 1024 * 1024));
}

void TestAppConfig::testSaveAndLoad()
{
    QTemporaryDir tmpDir;
    const QString path = tmpDir.filePath("test_config.json");

    // 写入
    auto &cfg = mc::AppConfig::instance();
    mc::AppConfigData data;
    data.defaultSerial = "SN12345";
    data.positionToUm = 0.05;
    data.minPositionMm = 1.0;
    data.maxPositionMm = 50.0;
    data.defaultMaxVelocity = 3000;
    data.defaultAcceleration = 5000;
    data.logLevel = "debug";
    data.cameraUserId = "CAM001";
    cfg.setData(data);
    cfg.saveAs(path);

    // 重新加载
    cfg.load(path);
    const auto &loaded = cfg.data();
    QCOMPARE(loaded.defaultSerial, QStringLiteral("SN12345"));
    QCOMPARE(loaded.positionToUm, 0.05);
    QCOMPARE(loaded.minPositionMm, 1.0);
    QCOMPARE(loaded.maxPositionMm, 50.0);
    QCOMPARE(loaded.defaultMaxVelocity, 3000);
    QCOMPARE(loaded.defaultAcceleration, 5000);
    QCOMPARE(loaded.logLevel, QStringLiteral("debug"));
    QCOMPARE(loaded.cameraUserId, QStringLiteral("CAM001"));
}

void TestAppConfig::testPartialLoad()
{
    QTemporaryDir tmpDir;
    const QString path = tmpDir.filePath("partial_config.json");

    // 写入一个只有部分字段的 JSON
    QFile f(path);
    f.open(QIODevice::WriteOnly);
    f.write(R"({
        "motor": { "defaultSerial": "SN999" },
        "log": { "level": "warn" }
    })");
    f.close();

    auto &cfg = mc::AppConfig::instance();
    // 重置为默认值，隔离前序测试（testSaveAndLoad）残留在单例中的状态
    cfg.setData(mc::AppConfigData{});
    cfg.load(path);
    const auto &d = cfg.data();
    // 已设置的字段
    QCOMPARE(d.defaultSerial, QStringLiteral("SN999"));
    QCOMPARE(d.logLevel, QStringLiteral("warn"));
    // 未设置的字段保持默认
    QCOMPARE(d.positionToUm, 0.029);
    QCOMPARE(d.minPositionMm, 0.005);
}

void TestAppConfig::testLevelFromString()
{
    using L = mc::LogLevel;
    QCOMPARE(mc::Logger::levelFromString("debug"), L::Debug);
    QCOMPARE(mc::Logger::levelFromString("info"), L::Info);
    QCOMPARE(mc::Logger::levelFromString("warn"), L::Warn);
    QCOMPARE(mc::Logger::levelFromString("warning"), L::Warn);
    QCOMPARE(mc::Logger::levelFromString("error"), L::Error);
    QCOMPARE(mc::Logger::levelFromString("unknown"), L::Info);
}

QTEST_APPLESS_MAIN(TestAppConfig)
#include "test_appconfig.moc"
