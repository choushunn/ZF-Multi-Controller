#pragma once

#include <QObject>
#include <QImage>
#include <QStringList>

namespace mc {

// ICamera：相机统一接口契约。
// DVP2 / TOUPCam 各自提供实现，UI 面向接口编程。
// 采集不应在 UI 线程同步执行，实现类内部负责后台采帧线程，
// 通过 frameReady 信号把帧回传 UI 刷新。
class ICamera : public QObject {
    Q_OBJECT

public:
    explicit ICamera(QObject *parent = nullptr) : QObject(parent) {}
    ~ICamera() override = default;

    // 设备发现：返回可用的相机标识列表（UserID / 序列号 / 友好名）
    virtual QStringList availableCameras() = 0;

    // 与 availableCameras() 顺序对应的设备型号名列表（用于 UI 展示）。
    // 默认与标识一致；具体相机实现可返回更友好的型号名。
    virtual QStringList availableCameraNames() { return availableCameras(); }

    // 连接管理
    virtual bool connectTo(const QString &id) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    // 采集控制
    virtual bool startCapture() = 0;
    virtual void stopCapture() = 0;
    virtual bool isCapturing() const = 0;

    // 保存当前帧到文件
    virtual bool saveFrame(const QString &filePath) = 0;

    // 相机信息
    virtual QString description() const = 0;  // 友好描述（型号/序列号）

signals:
    void frameReady(const QImage &frame);   // 后台线程采集到新帧
    void connectedChanged(bool connected);
    void logMessage(const QString &msg);     // 兼容现有日志模式
};

} // namespace mc
