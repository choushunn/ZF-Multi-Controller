#pragma once

#include "core/icamera.h"

#include <QLibrary>
#include <QImage>

#include <memory>

// ToupCamera 通过运行时加载 toupcam.dll 接入 ToupTek/同源相机。
// 不在源码树复制厂商 SDK；优先使用应用目录，再使用 TCC 的已安装 SDK。
class ToupCamera : public mc::ICamera {
    Q_OBJECT

public:
    explicit ToupCamera(QObject *parent = nullptr);
    ~ToupCamera() override;

    QStringList availableCameras() override;
    QStringList availableCameraNames() override;
    bool connectTo(const QString &id) override;
    void disconnect() override;
    bool isConnected() const override { return connected_; }

    // 相机参数（曝光/增益/分辨率）
    bool exposureRange(double *minMs, double *maxMs) override;
    bool setExposure(double ms) override;
    double exposure() const override;
    bool autoExposureEnabled() const override;
    bool setAutoExposure(bool on) override;
    bool gainRange(double *minPct, double *maxPct) override;
    bool setGain(double pct) override;
    double gain() const override;
    QStringList resolutions() const override;
    int currentResolution() const override;
    bool setResolution(int index) override;

    bool startCapture() override;
    void stopCapture() override;
    bool isCapturing() const override { return capturing_; }
    bool saveFrame(const QString &filePath) override;
    QString description() const override { return description_; }

private:
    struct Api;
    struct Device;
    struct FrameInfo;

    static void __stdcall onFrame(const void *data, const FrameInfo *info,
                                  int snap, void *context);
    bool ensureApi();

    std::unique_ptr<QLibrary> library_;
    std::unique_ptr<Api> api_;
    void *handle_ = nullptr;
    bool connected_ = false;
    bool capturing_ = false;
    QString description_;
    QImage lastFrame_;
    QStringList idList_;    // 与 nameList_ 逐项对应的设备标识（序列号/UserID）
    QStringList nameList_;  // 设备型号名（displayName）
};
