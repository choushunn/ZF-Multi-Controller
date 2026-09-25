#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>

namespace mc {

// AppConfig：配置管理对象，从 JSON 文件加载设备序列号、速度/加速度、
// 限位范围、日志级别与文件路径等参数，消除代码内硬编码。
// 用户在 GUI 中的设置可持久化并在下次启动恢复。
struct AppConfigData {
    // 电机
    QString defaultSerial;          // 默认设备序列号（空表示自动选择第一个）
    double positionToUm = 0.029;    // 设备单位 → 微米换算系数
    double minPositionMm = 0.005;   // 最小位置 mm
    double maxPositionMm = 24.5;    // 最大位置 mm
    int defaultMaxVelocity = 2600;  // 默认最大速度（设备单位）
    int defaultAcceleration = 4000; // 默认加速度（设备单位）

    // 日志
    QString logDir = QStringLiteral("logs");
    QString logLevel = QStringLiteral("info");
    qint64 logMaxFileSize = 5 * 1024 * 1024; // 5 MB

    // 相机
    QString cameraUserId;           // DVP2 相机的 UserID（空表示第一个）
    QString imageSaveDir = QStringLiteral("images");

    // 通用
    QString language = QStringLiteral("zh_CN");
};

class AppConfig : public QObject {
    Q_OBJECT

public:
    static AppConfig &instance();

    // 加载配置文件，路径默认为可执行文件旁的 config.json
    void load(const QString &path = QString());
    void save();
    void saveAs(const QString &path);

    // 获取/设置配置数据
    const AppConfigData &data() const { return data_; }
    void setData(const AppConfigData &data) { data_ = data; }

    QString configPath() const { return configPath_; }

    // 便捷访问
    const QString &defaultSerial() const { return data_.defaultSerial; }
    void setDefaultSerial(const QString &s) { data_.defaultSerial = s; }
    double positionToUm() const { return data_.positionToUm; }
    double minPositionMm() const { return data_.minPositionMm; }
    double maxPositionMm() const { return data_.maxPositionMm; }
    int defaultMaxVelocity() const { return data_.defaultMaxVelocity; }
    int defaultAcceleration() const { return data_.defaultAcceleration; }

signals:
    void configChanged();

private:
    AppConfig() = default;
    ~AppConfig() override = default;
    Q_DISABLE_COPY_MOVE(AppConfig)

    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);

    AppConfigData data_;
    QString configPath_;
};

} // namespace mc
